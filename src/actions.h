#pragma once

#include "boostdep.h"


#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <vector>

struct module_primary_actions
{
	virtual void heading(std::string const & module) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void header_start(std::string const & header) = 0;
	virtual void header_end(std::string const & header) = 0;

	virtual void from_header(std::string const & header) = 0;
};

struct module_primary_txt_actions : public module_primary_actions
{
	void heading(std::string const & module)
	{
		std::cout << "Primary dependencies for " << module << ":\n\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << module << ":\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void header_start(std::string const & header)
	{
		std::cout << "    <" << header << ">\n";
	}

	void header_end(std::string const & /*header*/)
	{
	}

	void from_header(std::string const & header)
	{
		std::cout << "        from <" << header << ">\n";
	}
};

struct module_primary_html_actions : public module_primary_actions
{
	void heading(std::string const & module)
	{
		std::cout << "\n\n<h1 id=\"primary-dependencies\">Primary dependencies for <em>" << module << "</em></h1>\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "  <h2 id=\"" << module << "\"><a href=\"" << module << ".html\"><em>" << module << "</em></a></h2>\n";
	}

	void module_end(std::string const & /*module*/)
	{
	}

	void header_start(std::string const & header)
	{
		std::cout << "    <h3><code>&lt;" << header << "&gt;</code></h3><ul>\n";
	}

	void header_end(std::string const & /*header*/)
	{
		std::cout << "    </ul>\n";
	}

	void from_header(std::string const & header)
	{
		std::cout << "      <li>from <code>&lt;" << header << "&gt;</code></li>\n";
	}
};

struct module_test_primary_actions : public module_primary_actions
{
	std::set< std::string > & m_;

	module_test_primary_actions(std::set< std::string > & m) : m_(m)
	{
	}

	void heading(std::string const & module)
	{
		std::cout << "Test dependencies for " << module << ":\n\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << module << "\n";
		m_.insert(module);
	}

	void module_end(std::string const & /*module*/)
	{
	}

	void header_start(std::string const & /*header*/)
	{
	}

	void header_end(std::string const & /*header*/)
	{
	}

	void from_header(std::string const & /*header*/)
	{
	}
};



struct module_secondary_actions
{
	virtual void heading(std::string const & module) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void module_adds(std::string const & module) = 0;
};

struct module_secondary_txt_actions : public module_secondary_actions
{
	void heading(std::string const & module)
	{
		std::cout << "Secondary dependencies for " << module << ":\n\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << module << ":\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void module_adds(std::string const & module)
	{
		std::cout << "    adds " << module << "\n";
	}
};

struct module_secondary_html_actions : public module_secondary_actions
{
	std::string m2_;

	void heading(std::string const & module)
	{
		std::cout << "\n\n<h1 id=\"secondary-dependencies\">Secondary dependencies for <em>" << module << "</em></h1>\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "  <h2><a href=\"" << module << ".html\"><em>" << module << "</em></a></h2><ul>\n";
		m2_ = module;
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "  </ul>\n";
	}

	void module_adds(std::string const & module)
	{
		std::cout << "    <li><a href=\"" << m2_ << ".html#" << module << "\">adds <em>" << module << "</em></a></li>\n";
	}
};


struct module_test_secondary_actions : public module_secondary_actions
{
	std::set< std::string > & m_;
	std::string m2_;

	module_test_secondary_actions(std::set< std::string > & m) : m_(m)
	{
	}

	void heading(std::string const & /*module*/)
	{
	}

	void module_start(std::string const & module)
	{
		m2_ = module;
	}

	void module_end(std::string const & /*module*/)
	{
	}

	void module_adds(std::string const & module)
	{
		if (m_.count(module) == 0)
		{
			std::cout << module << " (from " << m2_ << ")\n";
			m_.insert(module);
		}
	}
};

struct collect_primary_dependencies : public module_primary_actions
{
	std::set< std::string > set_;

	void heading(std::string const &)
	{
	}

	void module_start(std::string const & module)
	{
		if (module == "(unknown)") return;

		set_.insert(module);
	}

	void module_end(std::string const & /*module*/)
	{
	}

	void header_start(std::string const & /*header*/)
	{
	}

	void header_end(std::string const & /*header*/)
	{
	}

	void from_header(std::string const & /*header*/)
	{
	}
};

struct primary_pkgconfig_actions : public module_primary_actions
{
	std::string version_;
	std::string list_;

	void heading(std::string const &)
	{
	}

	void module_start(std::string const & module)
	{
		if (module == "(unknown)") return;

		std::string m2(module);
		std::replace(m2.begin(), m2.end(), '~', '_');

		if (!list_.empty())
		{
			list_ += ", ";
		}

		list_ += "boost_" + m2 + " = " + version_;
	}

	void module_end(std::string const &)
	{
	}

	void header_start(std::string const &)
	{
	}

	void header_end(std::string const &)
	{
	}

	void from_header(std::string const &)
	{
	}
};

struct module_subset_actions
{
	virtual void heading(std::string const & module) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void from_path(std::vector<std::string> const & path) = 0;
};

struct module_subset_txt_actions : public module_subset_actions
{
	void heading(std::string const & module)
	{
		std::cout << "Subset dependencies for " << module << ":\n\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << module << ":\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void from_path(std::vector<std::string> const & path)
	{
		for (std::vector<std::string>::const_iterator i = path.begin(); i != path.end(); ++i)
		{
			if (i == path.begin())
			{
				std::cout << "  ";
			}
			else
			{
				std::cout << " -> ";
			}

			std::cout << *i;
		}

		std::cout << "\n";
	}
};

struct module_subset_html_actions : public module_subset_actions
{
	void heading(std::string const & module)
	{
		std::cout << "\n\n<h1 id=\"subset-dependencies\">Subset dependencies for <em>" << module << "</em></h1>\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "  <h2 id=\"subset-" << module << "\"><a href=\"" << module << ".html\"><em>" << module << "</em></a></h2><ul>\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "</ul>\n";
	}

	void from_path(std::vector<std::string> const & path)
	{
		std::cout << "    <li>";

		for (std::vector<std::string>::const_iterator i = path.begin(); i != path.end(); ++i)
		{
			if (i != path.begin())
			{
				std::cout << " &#8674; ";
			}

			std::cout << "<code>" << *i << "</code>";
		}

		std::cout << "</li>\n";
	}
};

struct module_overview_actions
{
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void module2(std::string const & module) = 0;
};

struct list_dependencies_actions : public module_overview_actions
{
	void begin()
	{
	}

	void end()
	{
	}

	void module_start(std::string const & module)
	{
		std::cout << module << " ->";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void module2(std::string const & module)
	{
		if (module != "(unknown)")
		{
			std::cout << " " << module;
		}
	}
};


struct header_inclusion_actions
{
	virtual void heading(std::string const & header, std::string const & module) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void header(std::string const & header) = 0;
};

struct header_inclusion_txt_actions : public header_inclusion_actions
{
	void heading(std::string const & header, std::string const & module)
	{
		std::cout << "Inclusion report for <" << header << "> (in module " << module << "):\n\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "    from " << module << ":\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void header(std::string const & header)
	{
		std::cout << "        <" << header << ">\n";
	}
};

struct header_inclusion_html_actions : public header_inclusion_actions
{
	void heading(std::string const & header, std::string const & module)
	{
		std::cout << "<h1>Inclusion report for <code>&lt;" << header << "&gt;</code> (in module <em>" << module << "</em>)</h1>\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "  <h2>From <a href=\"" << module << ".html\"><em>" << module << "</em></a></h2><ul>\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "  </ul>\n";
	}

	void header(std::string const & header)
	{
		std::cout << "    <li><code>&lt;" << header << "&gt;</code></li>\n";
	}
};

struct module_reverse_actions
{
	virtual void heading(std::string const & module) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void header_start(std::string const & header) = 0;
	virtual void header_end(std::string const & header) = 0;

	virtual void from_header(std::string const & header) = 0;
};

struct module_reverse_txt_actions : public module_reverse_actions
{
	void heading(std::string const & module)
	{
		std::cout << "Reverse dependencies for " << module << ":\n\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << module << ":\n";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void header_start(std::string const & header)
	{
		std::cout << "    <" << header << ">\n";
	}

	void header_end(std::string const & /*header*/)
	{
	}

	void from_header(std::string const & header)
	{
		std::cout << "        from <" << header << ">\n";
	}
};

struct module_reverse_html_actions : public module_reverse_actions
{
	void heading(std::string const & module)
	{
		std::cout << "\n\n<h1 id=\"reverse-dependencies\">Reverse dependencies for <em>" << module << "</em></h1>\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "  <h2 id=\"reverse-" << module << "\"><a href=\"" << module << ".html\"><em>" << module << "</em></a></h2>\n";
	}

	void module_end(std::string const & /*module*/)
	{
	}

	void header_start(std::string const & header)
	{
		std::cout << "    <h3><code>&lt;" << header << "&gt;</code></h3><ul>\n";
	}

	void header_end(std::string const & /*header*/)
	{
		std::cout << "    </ul>\n";
	}

	void from_header(std::string const & header)
	{
		std::cout << "      <li>from <code>&lt;" << header << "&gt;</code></li>\n";
	}
};


struct module_level_actions
{
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual void level_start(int level) = 0;
	virtual void level_end(int level) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void module2(std::string const & module, int level) = 0;
};


int const unknown_level = INT_MAX / 2;

struct module_level_txt_actions : public module_level_actions
{
	int level_;

	void begin()
	{
		std::cout << "Module Levels:\n\n";
	}

	void end()
	{
	}

	void level_start(int level)
	{
		if (level >= unknown_level)
		{
			std::cout << "Level (undetermined):\n";
		}
		else
		{
			std::cout << "Level " << level << ":\n";
		}

		level_ = level;
	}

	void level_end(int /*level*/)
	{
		std::cout << "\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "    " << module;

		if (level_ > 0)
		{
			std::cout << " ->";
		}
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void module2(std::string const & module, int level)
	{
		std::cout << " " << module << "(";

		if (level >= unknown_level)
		{
			std::cout << "-";
		}
		else
		{
			std::cout << level;
		}

		std::cout << ")";
	}
};

struct module_level_html_actions : public module_level_actions
{
	int level_;

	void begin()
	{
		std::cout << "<div id='module-levels'><h1>Module Levels</h1>\n";
	}

	void end()
	{
		std::cout << "</div>\n";
	}

	void level_start(int level)
	{
		if (level >= unknown_level)
		{
			std::cout << "  <h2>Level <em>undetermined</em></h2>\n";
		}
		else
		{
			std::cout << "  <h2 id='level:" << level << "'>Level " << level << "</h2>\n";
		}

		level_ = level;
	}

	void level_end(int /*level*/)
	{
	}

	void module_start(std::string const & module)
	{
		std::cout << "    <h3 id='" << module << "'><a href=\"" << module << ".html\">" << module << "</a></h3><p class='primary-list'>";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "</p>\n";
	}

	void module2(std::string const & module, int level)
	{
		std::cout << " ";

		bool important = level < unknown_level && level > 1 && level >= level_ - 1;

		if (important)
		{
			std::cout << "<strong>";
		}

		std::cout << module;

		if (level < unknown_level)
		{
			std::cout << "<sup>" << level << "</sup>";
		}

		if (important)
		{
			std::cout << "</strong>";
		}
	}
};

struct module_weight_actions
{
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual void weight_start(int weight) = 0;
	virtual void weight_end(int weight) = 0;

	virtual void module_start(std::string const & module) = 0;
	virtual void module_end(std::string const & module) = 0;

	virtual void module_primary_start() = 0;
	virtual void module_primary(std::string const & module, int weight) = 0;
	virtual void module_primary_end() = 0;

	virtual void module_secondary_start() = 0;
	virtual void module_secondary(std::string const & module, int weight) = 0;
	virtual void module_secondary_end() = 0;
};

struct module_weight_txt_actions : public module_weight_actions
{
	void begin()
	{
		std::cout << "Module Weights:\n\n";
	}

	void end()
	{
	}

	void weight_start(int weight)
	{
		std::cout << "Weight " << weight << ":\n";
	}

	void weight_end(int /*weight*/)
	{
		std::cout << "\n";
	}

	void module_start(std::string const & module)
	{
		std::cout << "    " << module;
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void module_primary_start()
	{
		std::cout << " ->";
	}

	void module_primary(std::string const & module, int weight)
	{
		std::cout << " " << module << "(" << weight << ")";
	}

	void module_primary_end()
	{
	}

	void module_secondary_start()
	{
		std::cout << " ->";
	}

	void module_secondary(std::string const & module, int /*weight*/)
	{
		std::cout << " " << module;
	}

	void module_secondary_end()
	{
	}
};

struct module_weight_html_actions : public module_weight_actions
{
	int weight_;

	void begin()
	{
		std::cout << "<div id='module-weights'>\n<h1>Module Weights</h1>\n";
	}

	void end()
	{
		std::cout << "</div>\n";
	}

	void weight_start(int weight)
	{
		std::cout << "  <h2 id='weight:" << weight << "'>Weight " << weight << "</h2>\n";
		weight_ = weight;
	}

	void weight_end(int /*weight*/)
	{
	}

	void module_start(std::string const & module)
	{
		std::cout << "    <h3 id='" << module << "'><a href=\"" << module << ".html\">" << module << "</a></h3>";
	}

	void module_end(std::string const & /*module*/)
	{
		std::cout << "\n";
	}

	void module_primary_start()
	{
		std::cout << "<p class='primary-list'>";
	}

	void module_primary(std::string const & module, int weight)
	{
		std::cout << " ";

		bool heavy = weight >= 0.8 * weight_;

		if (heavy)
		{
			std::cout << "<strong>";
		}

		std::cout << module << "<sup>" << weight << "</sup>";

		if (heavy)
		{
			std::cout << "</strong>";
		}
	}

	void module_primary_end()
	{
		std::cout << "</p>";
	}

	void module_secondary_start()
	{
		std::cout << "<p class='secondary-list'>";
	}

	void module_secondary(std::string const & module, int /*weight*/)
	{
		std::cout << " " << module;
	}

	void module_secondary_end()
	{
		std::cout << "</p>";
	}
};


