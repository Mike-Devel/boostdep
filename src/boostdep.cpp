
// boostdep - a tool to generate Boost dependency reports
//
// Copyright 2014-2017 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include "boostdep.h"

#include "actions.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <climits>

namespace boost {
namespace dep {

// header -> module
static std::map< std::string, std::string > s_header_map;

// module -> headers
static std::map< std::string, std::set<std::string> > s_module_headers;

static std::set< std::string > s_modules;

const std::map< std::string, std::string >&				header_map() { return s_header_map; };
const std::map< std::string, std::set<std::string> >&	module_headers() { return s_module_headers; }
const std::set< std::string >&							modules() { return s_modules; }

static void scan_module_headers( fs::path const & path )
{
    try
    {
        std::string module = path.generic_string().substr( 5 ); // strip "libs/"

        std::replace( module.begin(), module.end(), '/', '~' );

        s_modules.insert( module );

        fs::path dir = path / "include";
        size_t n = dir.generic_string().size();

        fs::recursive_directory_iterator it( dir ), last;

        for( ; it != last; ++it )
        {
            if( fs::is_directory( it->status() ) )
            {
                continue;
            }

            std::string p2 = it->path().generic_string();
            p2 = p2.substr( n+1 );

            s_header_map[ p2 ] = module;
            s_module_headers[ module ].insert( p2 );
        }
    }
    catch( fs::filesystem_error const & x )
    {
        std::cout << x.what() << std::endl;
    }
}

static void scan_submodules( fs::path const & path )
{
    fs::directory_iterator it( path ), last;

    for( ; it != last; ++it )
    {
        fs::directory_entry const & e = *it;

        if( !fs::is_directory( it->status() ) )
        {
            continue;
        }

        fs::path path = e.path();

        if( fs::exists( path / "include" ) )
        {
            scan_module_headers( path );
        }

        if( fs::exists( path / "sublibs" ) )
        {
            scan_submodules( path );
        }
    }
}

void build_header_map()
{
    scan_submodules( "libs" );
}

static void scan_header_dependencies( std::string const & header, std::istream & is, std::map< std::string, std::set< std::string > > & deps, std::map< std::string, std::set< std::string > > & from )
{
    std::string line;

    while( std::getline( is, line ) )
    {
        while( !line.empty() && ( line[0] == ' ' || line[0] == '\t' ) )
        {
            line.erase( 0, 1 );
        }

        if( line.empty() || line[0] != '#' ) continue;

        line.erase( 0, 1 );

        while( !line.empty() && ( line[0] == ' ' || line[0] == '\t' ) )
        {
            line.erase( 0, 1 );
        }

        if( line.substr( 0, 7 ) != "include" ) continue;

        line.erase( 0, 7 );

        while( !line.empty() && ( line[0] == ' ' || line[0] == '\t' ) )
        {
            line.erase( 0, 1 );
        }

        if( line.size() < 2 ) continue;

        char ch = line[0];

        if( ch != '<' && ch != '"' ) continue;

        if( ch == '<' )
        {
            ch = '>';
        }

        line.erase( 0, 1 );

        std::string::size_type k = line.find_first_of( ch );

        if( k != std::string::npos )
        {
            line.erase( k );
        }

        std::map< std::string, std::string >::const_iterator i = s_header_map.find( line );

        if( i != s_header_map.end() )
        {
            deps[ i->second ].insert( line );
            from[ line ].insert( header );
        }
        else if( line.substr( 0, 6 ) == "boost/" )
        {
            deps[ "(unknown)" ].insert( line );
            from[ line ].insert( header );
        }
    }
}

static fs::path module_include_path( std::string module )
{
    std::replace( module.begin(), module.end(), '~', '/' );
    return fs::path( "libs" ) / module / "include";
}

static fs::path module_source_path( std::string module )
{
    std::replace( module.begin(), module.end(), '~', '/' );
    return fs::path( "libs" ) / module / "src";
}

static fs::path module_build_path( std::string module )
{
    std::replace( module.begin(), module.end(), '~', '/' );
    return fs::path( "libs" ) / module / "build";
}

static fs::path module_test_path( std::string module )
{
    std::replace( module.begin(), module.end(), '~', '/' );
    return fs::path( "libs" ) / module / "test";
}

static void scan_module_path( fs::path const & dir, bool remove_prefix, std::map< std::string, std::set< std::string > > & deps, std::map< std::string, std::set< std::string > > & from )
{
    size_t n = dir.generic_string().size();

    if( fs::exists( dir ) )
    {
        fs::recursive_directory_iterator it( dir ), last;

        for( ; it != last; ++it )
        {
            if( fs::is_directory( it->status() ) )
            {
                continue;
            }

            std::string header = it->path().generic_string();

            if( remove_prefix )
            {
                header = header.substr( n+1 );
            }

            ifstream_t is( it->path() );

            scan_header_dependencies( header, is, deps, from );
        }
    }
}

static void scan_module_dependencies( std::string const & module, module_primary_actions & actions, bool track_sources, bool track_tests, bool include_self )
{
    // module -> [ header, header... ]
    std::map< std::string, std::set< std::string > > deps;

    // header -> included from [ header, header... ]
    std::map< std::string, std::set< std::string > > from;

    scan_module_path( module_include_path( module ), true, deps, from );

    if( track_sources )
    {
        scan_module_path( module_source_path( module ), false, deps, from );
    }

    if( track_tests )
    {
        scan_module_path( module_test_path( module ), false, deps, from );
    }

    actions.heading( module );

    for( std::map< std::string, std::set< std::string > >::iterator i = deps.begin(); i != deps.end(); ++i )
    {
        if( i->first == module && !include_self ) continue;

        actions.module_start( i->first );

        for( std::set< std::string >::iterator j = i->second.begin(); j != i->second.end(); ++j )
        {
            actions.header_start( *j );

            std::set< std::string > const & f = from[ *j ];

            for( std::set< std::string >::const_iterator k = f.begin(); k != f.end(); ++k )
            {
                actions.from_header( *k );
            }

            actions.header_end( *j );
        }

        actions.module_end( i->first );
    }
}

// module depends on [ module, module... ]
static std::map< std::string, std::set< std::string > > s_module_deps;

// header is included by [header, header...]
static std::map< std::string, std::set< std::string > > s_header_deps;

// [ module, module... ] depend on module
static std::map< std::string, std::set< std::string > > s_reverse_deps;

// header includes [header, header...]
static std::map< std::string, std::set< std::string > > s_header_includes;

struct build_mdmap_actions: public module_primary_actions
{
    std::string module_;
    std::string module2_;
    std::string header_;

    void heading( std::string const & module )
    {
        module_ = module;
    }

    void module_start( std::string const & module )
    {
        if( module != module_ )
        {
            s_module_deps[ module_ ].insert( module );
            s_reverse_deps[ module ].insert( module_ );
        }

        module2_ = module;
    }

    void module_end( std::string const & /*module*/ )
    {
    }

    void header_start( std::string const & header )
    {
        header_ = header;
    }

    void header_end( std::string const & /*header*/ )
    {
    }

    void from_header( std::string const & header )
    {
        if( module_ != module2_ )
        {
            s_header_deps[ header_ ].insert( header );
        }

        s_header_includes[ header ].insert( header_ );
    }
};

static void build_module_dependency_map( bool track_sources, bool track_tests )
{
    for( std::set< std::string >::iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        build_mdmap_actions actions;
        scan_module_dependencies( *i, actions, track_sources, track_tests, true );
    }
}

static void output_module_primary_report( std::string const & module, module_primary_actions & actions, bool track_sources, bool track_tests )
{
    try
    {
        scan_module_dependencies( module, actions, track_sources, track_tests, false );
    }
    catch( fs::filesystem_error const & x )
    {
        std::cout << x.what() << std::endl;
    }
}

static void exclude( std::set< std::string > & x, std::set< std::string > const & y )
{
    for( std::set< std::string >::const_iterator i = y.begin(); i != y.end(); ++i )
    {
        x.erase( *i );
    }
}

static void output_module_secondary_report( std::string const & module, std::set< std::string> deps, module_secondary_actions & actions )
{
    actions.heading( module );

    deps.insert( module );

    // build transitive closure

    for( ;; )
    {
        std::set< std::string > deps2( deps );

        for( std::set< std::string >::iterator i = deps.begin(); i != deps.end(); ++i )
        {
            std::set< std::string > deps3 = s_module_deps[ *i ];

            exclude( deps3, deps );

            if( deps3.empty() )
            {
                continue;
            }

            actions.module_start( *i );

            for( std::set< std::string >::iterator j = deps3.begin(); j != deps3.end(); ++j )
            {
                actions.module_adds( *j );
            }

            actions.module_end( *i );

            deps2.insert( deps3.begin(), deps3.end() );
        }

        if( deps == deps2 )
        {
            break;
        }
        else
        {
            deps = deps2;
        }
    }
}

static void output_module_secondary_report( std::string const & module, module_secondary_actions & actions )
{
    output_module_secondary_report( module, s_module_deps[ module ], actions );
}

static void output_header_inclusion_report( std::string const & header, header_inclusion_actions & actions )
{
    std::string module = s_header_map[ header ];

    actions.heading( header, module );

    std::set< std::string > from = s_header_deps[ header ];

    // classify 'from' dependencies by module

    // module -> [header, header...]
    std::map< std::string, std::set< std::string > > from2;

    for( std::set< std::string >::iterator i = from.begin(); i != from.end(); ++i )
    {
        from2[ s_header_map[ *i ] ].insert( *i );
    }

    for( std::map< std::string, std::set< std::string > >::iterator i = from2.begin(); i != from2.end(); ++i )
    {
        actions.module_start( i->first );

        for( std::set< std::string >::iterator j = i->second.begin(); j != i->second.end(); ++j )
        {
            actions.header( *j );
        }

        actions.module_end( i->first );
    }
}

// output_module_primary_report

void output_module_primary_report( std::string const & module, bool html, bool track_sources, bool track_tests )
{
    if( html )
    {
        module_primary_html_actions actions;
        output_module_primary_report( module, actions, track_sources, track_tests );
    }
    else
    {
        module_primary_txt_actions actions;
        output_module_primary_report( module, actions, track_sources, track_tests );
    }
}

// output_module_secondary_report

void output_module_secondary_report( std::string const & module, bool html )
{
    if( html )
    {
        module_secondary_html_actions actions;
        output_module_secondary_report( module, actions );
    }
    else
    {
        module_secondary_txt_actions actions;
        output_module_secondary_report( module, actions );
    }
}

// output_header_report

void output_header_report( std::string const & header, bool html )
{
    if( html )
    {
        header_inclusion_html_actions actions;
        output_header_inclusion_report( header, actions );
    }
    else
    {
        header_inclusion_txt_actions actions;
        output_header_inclusion_report( header, actions );
    }
}

// output_module_reverse_report

static void output_module_reverse_report( std::string const & module, module_reverse_actions & actions )
{
    actions.heading( module );

    std::set< std::string > const from = s_reverse_deps[ module ];

    for( std::set< std::string >::const_iterator i = from.begin(); i != from.end(); ++i )
    {
        actions.module_start( *i );

        for( std::map< std::string, std::set< std::string > >::iterator j = s_header_deps.begin(); j != s_header_deps.end(); ++j )
        {
            if( s_header_map[ j->first ] == module )
            {
                bool header_started = false;

                for( std::set< std::string >::iterator k = j->second.begin(); k != j->second.end(); ++k )
                {
                    if( s_header_map[ *k ] == *i )
                    {
                        if( !header_started )
                        {
                            actions.header_start( j->first );

                            header_started = true;
                        }

                        actions.from_header( *k );
                    }
                }

                if( header_started )
                {
                    actions.header_end( j->first );
                }
            }
        }

        actions.module_end( *i );
    }
}



void output_module_reverse_report( std::string const & module, bool html )
{
    if( html )
    {
        module_reverse_html_actions actions;
        output_module_reverse_report( module, actions );
    }
    else
    {
        module_reverse_txt_actions actions;
        output_module_reverse_report( module, actions );
    }
}

// module_level_report

static void output_module_level_report( module_level_actions & actions )
{
    // build module level map

    std::map< std::string, int > level_map;

    for( std::set< std::string >::iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        if( s_module_deps[ *i ].empty() )
        {
            level_map[ *i ] = 0;
            // std::cerr << *i << ": " << 0 << std::endl;
        }
        else
        {
            level_map[ *i ] = unknown_level;
        }
    }

    // build transitive closure to see through cycles

    std::map< std::string, std::set< std::string > > deps2 = s_module_deps;

    {
        bool done;

        do
        {
            done = true;

            for( std::map< std::string, std::set< std::string > >::iterator i = deps2.begin(); i != deps2.end(); ++i )
            {
                std::set< std::string > tmp = i->second;

                for( std::set< std::string >::iterator j = i->second.begin(); j != i->second.end(); ++j )
                {
                    std::set< std::string > tmp2 = deps2[ *j ];
                    tmp.insert( tmp2.begin(), tmp2.end() );
                }

                if( tmp.size() != i->second.size() )
                {
                    i->second = tmp;
                    done = false;
                }
            }
        }
        while( !done );
    }

    // compute acyclic levels

    for( int k = 1, n = s_modules.size(); k < n; ++k )
    {
        for( std::map< std::string, std::set< std::string > >::iterator i = s_module_deps.begin(); i != s_module_deps.end(); ++i )
        {
            // i->first depends on i->second

            if( level_map[ i->first ] >= unknown_level )
            {
                int level = 0;

                for( std::set< std::string >::iterator j = i->second.begin(); j != i->second.end(); ++j )
                {
                    level = std::max( level, level_map[ *j ] + 1 );
                }

                if( level == k )
                {
                    level_map[ i->first ] = level;
                    // std::cerr << i->first << ": " << level << std::endl;
                }
            }
        }
    }

    // min_level_map[ M ] == L means the level is unknown, but at least L
    std::map< std::string, int > min_level_map;

    // initialize min_level_map for acyclic dependencies

    for( std::map< std::string, int >::iterator i = level_map.begin(); i != level_map.end(); ++i )
    {
        if( i->second < unknown_level )
        {
            min_level_map[ i->first ] = i->second;
        }
    }

    // compute levels for cyclic modules

    for( int k = 1, n = s_modules.size(); k < n; ++k )
    {
        for( std::map< std::string, std::set< std::string > >::iterator i = s_module_deps.begin(); i != s_module_deps.end(); ++i )
        {
            if( level_map[ i->first ] >= unknown_level )
            {
                int level = 0;

                for( std::set< std::string >::iterator j = i->second.begin(); j != i->second.end(); ++j )
                {
                    int jl = level_map[ *j ];

                    if( jl < unknown_level )
                    {
                        level = std::max( level, jl + 1 );
                    }
                    else
                    {
                        int ml = min_level_map[ *j ];

                        if( deps2[ *j ].count( i->first ) == 0 )
                        {
                            // *j does not depend on i->first, so
                            // the level of i->first is at least
                            // 1 + the minimum level of *j

                            ++ml;
                        }

                        level = std::max( level, ml );
                    }
                }

                min_level_map[ i->first ] = level;
            }
        }
    }

    // reverse level map

    std::map< int, std::set< std::string > > reverse_level_map;

    for( std::map< std::string, int >::iterator i = level_map.begin(); i != level_map.end(); ++i )
    {
        int level = i->second;

        if( level >= unknown_level )
        {
            int min_level = min_level_map[ i->first ];

            if( min_level != 0 )
            {
                level = min_level;
            }
        }

        reverse_level_map[ level ].insert( i->first );
    }

    // output report

    actions.begin();

    for( std::map< int, std::set< std::string > >::iterator i = reverse_level_map.begin(); i != reverse_level_map.end(); ++i )
    {
        actions.level_start( i->first );

        for( std::set< std::string >::iterator j = i->second.begin(); j != i->second.end(); ++j )
        {
            actions.module_start( *j );

            std::set< std::string > mdeps = s_module_deps[ *j ];

            for( std::set< std::string >::iterator k = mdeps.begin(); k != mdeps.end(); ++k )
            {
                int level = level_map[ *k ];

                if( level >= unknown_level )
                {
                    int min_level = min_level_map[ *k ];

                    if( min_level != 0 )
                    {
                        level = min_level;
                    }
                }

                actions.module2( *k, level );
            }

            actions.module_end( *j );
        }

        actions.level_end( i->first );
    }

    actions.end();
}



void output_module_level_report( bool html )
{
    if( html )
    {
        module_level_html_actions actions;
        output_module_level_report( actions );
    }
    else
    {
        module_level_txt_actions actions;
        output_module_level_report( actions );
    }
}

// module_overview_report

static void output_module_overview_report( module_overview_actions & actions )
{
    actions.begin();

    for( std::set< std::string >::iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        actions.module_start( *i );

        std::set< std::string > const mdeps = s_module_deps[ *i ];

        for( std::set< std::string >::const_iterator j = mdeps.begin(); j != mdeps.end(); ++j )
        {
            actions.module2( *j );
        }

        actions.module_end( *i );
    }

    actions.end();
}

struct module_overview_txt_actions: public module_overview_actions
{
    bool deps_;

    void begin()
    {
        std::cout << "Module Overview:\n\n";
    }

    void end()
    {
    }

    void module_start( std::string const & module )
    {
        std::cout << module;
        deps_ = false;
    }

    void module_end( std::string const & /*module*/ )
    {
        std::cout << "\n";
    }

    void module2( std::string const & module )
    {
        if( !deps_ )
        {
            std::cout << " ->";
            deps_ = true;
        }

        std::cout << " " << module;
    }
};

struct module_overview_html_actions: public module_overview_actions
{
    void begin()
    {
        std::cout << "<div id='module-overview'><h1>Module Overview</h1>\n";
    }

    void end()
    {
        std::cout << "</div>\n";
    }

    void module_start( std::string const & module )
    {
        std::cout << "  <h2 id='" << module << "'><a href=\"" << module << ".html\"><em>" << module << "</em></a></h2><p class='primary-list'>";
    }

    void module_end( std::string const & /*module*/ )
    {
        std::cout << "</p>\n";
    }

    void module2( std::string const & module )
    {
        std::cout << " " << module;
    }
};

void output_module_overview_report( bool html )
{
    if( html )
    {
        module_overview_html_actions actions;
        output_module_overview_report( actions );
    }
    else
    {
        module_overview_txt_actions actions;
        output_module_overview_report( actions );
    }
}

// list_dependencies

void list_dependencies()
{
    list_dependencies_actions actions;
    output_module_overview_report( actions );
}

//

void output_html_header( std::string const & title, std::string const & stylesheet, std::string const & prefix )
{
    std::cout << "<html>\n";
    std::cout << "<head>\n";
    std::cout << "<title>" << title << "</title>\n";

    if( !stylesheet.empty() )
    {
        std::cout << "<link rel=\"stylesheet\" type=\"text/css\" href=\"" << stylesheet << "\" />\n";
    }

    std::cout << "</head>\n";
    std::cout << "<body>\n";

    if( !prefix.empty() )
    {
        std::cout << prefix << std::endl;
    }
}

void output_html_footer( std::string const & footer )
{
    std::cout << "<hr />\n";
    std::cout << "<p class=\"footer\">" << footer << "</p>\n";
    std::cout << "</body>\n";
    std::cout << "</html>\n";
}

void enable_secondary( bool & secondary, bool track_sources, bool track_tests )
{
    if( !secondary )
    {
        try
        {
            build_module_dependency_map( track_sources, track_tests );
        }
        catch( fs::filesystem_error const & x )
        {
            std::cout << x.what() << std::endl;
        }

        secondary = true;
    }
}

void list_modules()
{
    for( std::set< std::string >::iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        std::cout << *i << "\n";
    }
}

void list_buildable()
{
    for( std::set< std::string >::iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        if( fs::exists( module_build_path( *i ) ) && fs::exists( module_source_path( *i ) ) )
        {
            std::cout << *i << "\n";
        }
    }
}

// module_weight_report

static void output_module_weight_report( module_weight_actions & actions )
{
    // gather secondary dependencies

    struct build_secondary_deps: public module_secondary_actions
    {
        std::map< std::string, std::set< std::string > > * pm_;

        build_secondary_deps( std::map< std::string, std::set< std::string > > * pm ): pm_( pm )
        {
        }

        std::string module_;

        void heading( std::string const & module )
        {
            module_ = module;
        }

        void module_start( std::string const & /*module*/ )
        {
        }

        void module_end( std::string const & /*module*/ )
        {
        }

        void module_adds( std::string const & module )
        {
            (*pm_)[ module_ ].insert( module );
        }
    };

    std::map< std::string, std::set< std::string > > secondary_deps;

    build_secondary_deps bsd( &secondary_deps );

    for( std::set< std::string >::const_iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        output_module_secondary_report( *i, bsd );
    }

    // build weight map

    std::map< int, std::set< std::string > > modules_by_weight;

    for( std::set< std::string >::const_iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        int w = s_module_deps[ *i ].size() + secondary_deps[ *i ].size();
        modules_by_weight[ w ].insert( *i );
    }

    // output report

    actions.begin();

    for( std::map< int, std::set< std::string > >::const_iterator i = modules_by_weight.begin(); i != modules_by_weight.end(); ++i )
    {
        actions.weight_start( i->first );

        for( std::set< std::string >::const_iterator j = i->second.begin(); j != i->second.end(); ++j )
        {
            actions.module_start( *j );

            if( !s_module_deps[ *j ].empty() )
            {
                actions.module_primary_start();

                for( std::set< std::string >::const_iterator k = s_module_deps[ *j ].begin(); k != s_module_deps[ *j ].end(); ++k )
                {
                    int w = s_module_deps[ *k ].size() + secondary_deps[ *k ].size();
                    actions.module_primary( *k, w );
                }

                actions.module_primary_end();
            }

            if( !secondary_deps[ *j ].empty() )
            {
                actions.module_secondary_start();

                for( std::set< std::string >::const_iterator k = secondary_deps[ *j ].begin(); k != secondary_deps[ *j ].end(); ++k )
                {
                    int w = s_module_deps[ *k ].size() + secondary_deps[ *k ].size();
                    actions.module_secondary( *k, w );
                }

                actions.module_secondary_end();
            }

            actions.module_end( *j );
        }

        actions.weight_end( i->first );
    }

    actions.end();
}



void output_module_weight_report( bool html )
{
    if( html )
    {
        module_weight_html_actions actions;
        output_module_weight_report( actions );
    }
    else
    {
        module_weight_txt_actions actions;
        output_module_weight_report( actions );
    }
}

// output_module_subset_report

void add_module_headers( fs::path const & dir, std::set<std::string> & headers )
{
    if( fs::exists( dir ) )
    {
        fs::recursive_directory_iterator it( dir ), last;

        for( ; it != last; ++it )
        {
            if( fs::is_directory(it->status()) )
            {
                continue;
            }

            headers.insert( it->path().generic_string() );
        }
    }
}

static void output_module_subset_report_( std::string const & module, std::set<std::string> const & headers, module_subset_actions & actions )
{
    // build header closure

    // header -> (header)*
    std::map< std::string, std::set<std::string> > inc2;

    // (header, header) -> path
    std::map< std::pair<std::string, std::string>, std::vector<std::string> > paths;

    for( std::set<std::string>::const_iterator i = headers.begin(); i != headers.end(); ++i )
    {
        std::set<std::string> & s = inc2[ *i ];

        s = s_header_includes[ *i ];

        for( std::set<std::string>::const_iterator j = s.begin(); j != s.end(); ++j )
        {
            std::vector<std::string> & v = paths[ std::make_pair( *i, *j ) ];

            v.resize( 0 );
            v.push_back( *i );
            v.push_back( *j );
        }
    }

    for( ;; )
    {
        bool r = false;

        for( std::map< std::string, std::set<std::string> >::iterator i = inc2.begin(); i != inc2.end(); ++i )
        {
            std::set<std::string> & s2 = i->second;

            for( std::set<std::string>::const_iterator j = s2.begin(); j != s2.end(); ++j )
            {
                std::set<std::string> const & s = s_header_includes[ *j ];

                for( std::set<std::string>::const_iterator k = s.begin(); k != s.end(); ++k )
                {
                    if( s2.count( *k ) == 0 )
                    {
                        s2.insert( *k );

                        std::vector<std::string> const & v1 = paths[ std::make_pair( i->first, *j ) ];
                        std::vector<std::string> & v2 = paths[ std::make_pair( i->first, *k ) ];

                        v2 = v1;
                        v2.push_back( *k );

                        r = true;
                    }
                }
            }
        }

        if( !r ) break;
    }

    // module -> header -> path [header -> header -> header]
    std::map< std::string, std::map< std::string, std::vector<std::string> > > subset;

    for( std::set<std::string>::const_iterator i = headers.begin(); i != headers.end(); ++i )
    {
        std::set<std::string> const & s = inc2[ *i ];

        for( std::set<std::string>::const_iterator j = s.begin(); j != s.end(); ++j )
        {
            std::string const & m = s_header_map[ *j ];

            if( m.empty() ) continue;

            std::vector<std::string> const & path = paths[ std::make_pair( *i, *j ) ];

            if( subset.count( m ) == 0 || subset[ m ].count( *i ) == 0 || subset[ m ][ *i ].size() > path.size() )
            {
                subset[ m ][ *i ] = path;
            }
        }
    }

    actions.heading( module );

    for( std::map< std::string, std::map< std::string, std::vector<std::string> > >::const_iterator i = subset.begin(); i != subset.end(); ++i )
    {
        if( i->first == module ) continue;

        actions.module_start( i->first );

        int k = 0;

        for( std::map< std::string, std::vector<std::string> >::const_iterator j = i->second.begin(); j != i->second.end() && k < 4; ++j, ++k )
        {
            actions.from_path( j->second );
        }

        actions.module_end( i->first );
    }
}

static void output_module_subset_report( std::string const & module, bool track_sources, bool track_tests, module_subset_actions & actions )
{
    std::set<std::string> headers = s_module_headers[ module ];

    if( track_sources )
    {
        add_module_headers( module_source_path( module ), headers );
    }

    if( track_tests )
    {
        add_module_headers( module_test_path( module ), headers );
    }

    output_module_subset_report_( module, headers, actions );
}

void output_module_subset_report( std::string const & module, bool track_sources, bool track_tests, bool html )
{
    if( html )
    {
        module_subset_html_actions actions;
        output_module_subset_report( module, track_sources, track_tests, actions );
    }
    else
    {
        module_subset_txt_actions actions;
        output_module_subset_report( module, track_sources, track_tests, actions );
    }
}

// --list-exceptions

void list_exceptions()
{
    std::string lm;

    for( std::map< std::string, std::set<std::string> >::const_iterator i = s_module_headers.begin(); i != s_module_headers.end(); ++i )
    {
        std::string module = i->first;

        std::replace( module.begin(), module.end(), '~', '/' );

        std::string const prefix = "boost/" + module;
        size_t const n = prefix.size();

        for( std::set< std::string >::const_iterator j = i->second.begin(); j != i->second.end(); ++j )
        {
            std::string const & header = *j;

            if( header.substr( 0, n+1 ) != prefix + '/' && header != prefix + ".hpp" )
            {
                if( lm != module )
                {
                    std::cout << module << ":\n";
                    lm = module;
                }

                std::cout << "  " << header << '\n';
            }
        }
    }
}

// --test

void output_module_test_report( std::string const & module )
{
    std::set< std::string > m;

    module_test_primary_actions a1( m );
    output_module_primary_report( module, a1, true, true );

    std::cout << "\n";

    bool secondary = false;
    enable_secondary( secondary, true, false );

    std::set< std::string > m2( m );
    m2.insert( module );

    module_test_secondary_actions a2( m2 );

    output_module_secondary_report( module, m, a2 );
}

// --cmake

static std::string module_cmake_package( std::string module )
{
    std::replace( module.begin(), module.end(), '~', '_' );
    return "boost_" + module;
}

static std::string module_cmake_target( std::string module )
{
    std::replace( module.begin(), module.end(), '~', '_' );
    return "boost::" + module;
}

void output_module_cmake_report( std::string module )
{
    std::replace( module.begin(), module.end(), '/', '~' );

    std::cout << "# Generated file. Do not edit.\n\n";

    collect_primary_dependencies a1;
    output_module_primary_report( module, a1, false, false );

    if( !fs::exists( module_source_path( module ) ) )
    {
        for( std::set< std::string >::const_iterator i = a1.set_.begin(); i != a1.set_.end(); ++i )
        {
            std::cout << "boost_declare_dependency(" << module_cmake_package( *i ) << " INTERFACE " << module_cmake_target( *i ) << ")\n";
        }
    }
    else
    {
        collect_primary_dependencies a2;
        output_module_primary_report( module, a2, true, false );

        for( std::set< std::string >::const_iterator i = a1.set_.begin(); i != a1.set_.end(); ++i )
        {
            a2.set_.erase( *i );
            std::cout << "boost_declare_dependency(" << module_cmake_package( *i ) << " PUBLIC " << module_cmake_target( *i ) << ")\n";
        }

        std::cout << "\n";

        for( std::set< std::string >::const_iterator i = a2.set_.begin(); i != a2.set_.end(); ++i )
        {
            std::cout << "boost_declare_dependency(" << module_cmake_package( *i ) << " PRIVATE " << module_cmake_target( *i ) << ")\n";
        }
    }
}

// --list-missing-headers

struct missing_header_actions: public module_primary_actions
{
    std::string module_, module2_;

    void heading( std::string const & module )
    {
        module_ = module;
    }

    void module_start( std::string const & module )
    {
        module2_ = module;
    }

    void module_end( std::string const & /*module*/ )
    {
    }

    void header_start( std::string const & header )
    {
        if( module2_ == "(unknown)" )
        {
            if( !module_.empty() )
            {
                std::cout << module_ << ":\n";
                module_.clear();
            }

            std::cout << "    <" << header << ">\n";
        }
    }

    void header_end( std::string const & /*header*/ )
    {
    }

    void from_header( std::string const & header )
    {
        if( module2_ == "(unknown)" )
        {
            std::cout << "        from <" << header << ">\n";
        }
    }
};

static void list_missing_headers( std::string const & module )
{
    missing_header_actions a;
    output_module_primary_report( module, a, false, false );
}

void list_missing_headers()
{
    for( std::set< std::string >::const_iterator i = s_modules.begin(); i != s_modules.end(); ++i )
    {
        list_missing_headers( *i );
    }
}

// --pkgconfig

static void output_requires( std::string const & section, std::string const & version, std::set< std::string > const & s )
{
    bool first = true;

    for( std::set< std::string >::const_iterator i = s.begin(); i != s.end(); ++i )
    {
        if( first )
        {
            std::cout << section << ": ";
            first = false;
        }
        else
        {
            std::cout << ", ";
        }

        std::string m2( *i );
        std::replace( m2.begin(), m2.end(), '~', '_' );

        std::cout << "boost_" << m2 << " = " << version;
    }
}

void output_pkgconfig( std::string const & module, std::string const & version, int argc, char const* argv[] )
{
    for( int i = 0; i < argc; ++i )
    {
        std::cout << argv[ i ] << '\n';
    }

    std::cout << '\n';

    std::string m2( module );
    std::replace( m2.begin(), m2.end(), '/', '_' );

    std::string m3( module );
    std::replace( m3.begin(), m3.end(), '/', '~' );

    std::cout << "Name: boost_" << module << '\n';
    std::cout << "Description: Boost C++ library '" << module << "'\n";
    std::cout << "Version: " << version << '\n';
    std::cout << "URL: http://www.boost.org/libs/" << module << '\n';
    std::cout << "Cflags: -I${includedir}\n";

    if( fs::exists( module_build_path( module ) ) && fs::exists( module_source_path( module ) ) )
    {
        std::cout << "Libs: -L${libdir} -lboost_" << m2 << "\n";
    }

    collect_primary_dependencies a1;
    output_module_primary_report( m3, a1, false, false );

    if( !a1.set_.empty() )
    {
        output_requires( "Requires", version, a1.set_ );
        std::cout << std::endl;
    }

    collect_primary_dependencies a2;
    output_module_primary_report( m3, a2, true, false );

    for( std::set< std::string >::const_iterator i = a1.set_.begin(); i != a1.set_.end(); ++i )
    {
        a2.set_.erase( *i );
    }

    if( !a2.set_.empty() )
    {
        output_requires( "Requires.private", version, a2.set_ );
        std::cout << std::endl;
    }
}

// --subset-for

void output_directory_subset_report( std::string const & module, std::set<std::string> const & headers, bool html )
{
    for( std::set<std::string>::const_iterator i = headers.begin(); i != headers.end(); ++i )
    {
        std::map< std::string, std::set< std::string > > deps;
        std::map< std::string, std::set< std::string > > from;

        std::ifstream is( i->c_str() );
        scan_header_dependencies( *i, is, deps, from );

        for( std::map< std::string, std::set< std::string > >::const_iterator j = from.begin(); j != from.end(); ++j )
        {
            for( std::set<std::string>::const_iterator k = j->second.begin(); k != j->second.end(); ++k )
            {
                s_header_includes[ *k ].insert( j->first );
            }
        }
    }

    if( html )
    {
        module_subset_html_actions actions;
        output_module_subset_report_( module, headers, actions );
    }
    else
    {
        module_subset_txt_actions actions;
        output_module_subset_report_( module, headers, actions );
    }
}

//

bool find_boost_root()
{
    for( int i = 0; i < 32; ++i )
    {
        if( fs::exists( "Jamroot" ) )
        {
            return true;
        }

        fs::path p = fs::current_path();

        if( p == p.root_path() )
        {
            return false;
        }

        fs::current_path( p.parent_path() );
    }

    return false;
}

}
}

