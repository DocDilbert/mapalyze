#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <cxxopts.hpp>
#include <string>
#include <fstream>
#include <streambuf>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;

enum SectionType
{
  LOAD,
  SECTION,
  EMPTY,
  UNKNOWN
};

struct Section
{
  string section_name;
  string adress;
  string size;
  long start_pos;
  long end_pos;
};

void ParseObjectLine(const std::string &str)
{
  cout << str << endl;
}
SectionType IdentifyLine(const std::string &str)
{
  if (boost::starts_with(str, "LOAD"))
  {
    return SectionType::LOAD;
  }
  if (boost::starts_with(str, "\r"))
  {
    return SectionType::EMPTY;
  }

  return SectionType::SECTION;
}

unsigned count_trailing_spaces(const string &str)
{
  for (unsigned i = 0; i < str.length(); i++)
  {
    if (str[i] != ' ')
    {
      return i;
    }
  }
  return str.length();
}

std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return (contents);
  }
  throw(errno);
}

string remove_trailing_cr(string str)
{
  auto newline_pos = str.find("\r");
  if (newline_pos != string::npos)
  {
    str = str.substr(0, newline_pos);
  }

  return str;
}
Section parse_section(const string &line, unsigned start_pos)
{
  vector<string> container;
  auto splitted = boost::split(container, line, boost::is_any_of(" "), boost::token_compress_on);

  // remove trailing newline if it exists
  auto size = remove_trailing_cr(splitted[2]);

  Section new_sec = {
      .section_name = splitted[0],
      .adress = splitted[1],
      .size = size,
      .start_pos = start_pos,
      .end_pos = -1 // updated later
  };

  return new_sec;
}

ostream &operator<<(ostream &os, const Section &msec)
{
  os.width(20);

  os << left << msec.section_name
     << " " << msec.adress << " ";
  os.width(10);
  os << left << msec.size
     << " " << msec.start_pos
     << " " << msec.end_pos;
  return os;
}

vector<Section> parse_sections(string const &str)
{
  const char separator = '\n';
  std::string::const_iterator start = str.begin();
  std::string::const_iterator end = str.end();
  std::string::const_iterator next = std::find(start, end, separator);

  vector<Section> sections;

  while (next != end)
  {
    auto line = std::string(start, next);
    if (count_trailing_spaces(line) == 0)
    {
      if (IdentifyLine(line) == SectionType::SECTION)
      {
        sections.push_back(parse_section(line, start - str.begin()));
      }
    }
    start = next + 1;
    next = std::find(start, end, separator);
  }

  // update end positions
  for (int i = 0; i < sections.size() - 1; i++)
  {
    sections[i].end_pos = sections[i + 1].start_pos - 3;
  }
  sections[sections.size() - 1].end_pos = str.size() - 1;
  return sections;
}

bool is_filter(const string &line)
{
  auto l = remove_trailing_cr(line);
  boost::trim(l);

  if ((boost::starts_with(l, "*(")) && (boost::ends_with(l, ")")))
  {
    return true;
  }

  return false;
}

struct Filter
{
  Section section;
  string filter;
  long start_pos;
  long end_pos;
};

ostream &operator<<(ostream &os, const Filter &filter)
{
  os.width(20);
  os << left << filter.section.section_name << " ";
  os << left << filter.filter;

  os.width(10);
  os << " " << filter.start_pos
     << " " << filter.end_pos;

  return os;
}

Filter parse_filter(const string &line, Section sec, unsigned start_pos)
{
  auto l = remove_trailing_cr(line);
  boost::trim(l);

  Filter new_filter = {
      .section = sec,
      .filter = l,
      .start_pos = start_pos,
      .end_pos = -1};

  return new_filter;
}

vector<Filter> parse_filters(string const &str, const Section &sec)
{
  const char separator = '\n';
  std::string::const_iterator start = str.begin() + sec.start_pos;
  std::string::const_iterator end = str.begin() + sec.end_pos;
  std::string::const_iterator next = std::find(start, end, separator);

  vector<Filter> filters;
  while (next < end)
  {
    auto line = std::string(start, next);
    // reduce solutions
    if (count_trailing_spaces(line) == 1)
    {
      if (is_filter(line))
      {
        filters.push_back(parse_filter(line, sec, start - str.begin()));
      }
    }

    start = next + 1;
    next = std::find(start, end, separator);
  }
  // update end positions
  if (filters.size() > 1)
  {
    for (int i = 0; i < filters.size() - 1; i++)
    {
      filters[i].end_pos = filters[i + 1].start_pos - 2;
    }
  }
  if (filters.size() > 0)
  {
    filters[filters.size() - 1].end_pos = sec.end_pos - 1;
  }

  return filters;
}

struct Object
{
  string secname;
  string filename;
  string address;
  string size;
};

ostream &operator<<(ostream &os, const Object &obj)
{
  os.width(45);
  os << left << obj.filename;
  os << left << obj.address << " ";
  os << left << obj.size;
  //os << left << obj.secname << " ";
  return os;
}

Object parse_object(vector<string> elements)
{
  string filename;
  if (boost::starts_with(elements[0], "*fill*"))
  {
    filename = "";
  }
  else
  {
    auto path_fn = elements[3];
    filename = path_fn.substr(path_fn.find_last_of("\\/") + 1, string::npos);
  }
  Object obj = {
      .secname = elements[0],
      .address = elements[1],
      .size = elements[2],
      .filename = filename
  };

  return obj;
}
vector<Object> parse_objects(string const &str, const Filter &filter)
{
  const char separator = '\n';
  std::string::const_iterator start = str.begin() + filter.start_pos;
  std::string::const_iterator end = str.begin() + filter.end_pos;
  std::string::const_iterator next = std::find(start, end, separator);
  start = next + 1;
  next = std::find(start, end, separator); // ignore first line because this is the line which contains the filter

  vector<Object> objects;
  while (next < end)
  {
    auto line = std::string(start, next);

    // reduce solutions
    if (count_trailing_spaces(line) == 1)
    {
      line = remove_trailing_cr(line);
      boost::trim(line);
      vector<string> container;
      auto splitted = boost::split(container, line, boost::is_any_of(" "), boost::token_compress_on);

      if (splitted.size() == 1)
      { // line is incompelete ... search for content on new line
        start = next + 1;
        next = std::find(start, end, separator);
        line = std::string(start, next);
        line = remove_trailing_cr(line);
        boost::trim(line);
        vector<string> container2;
        auto splitted2 = boost::split(container, line, boost::is_any_of(" "), boost::token_compress_on);

        splitted.insert(splitted.end(), splitted2.begin(), splitted2.end());
      }

      objects.push_back(parse_object(splitted));
    }

    start = next + 1;
    next = std::find(start, end, separator);
  }

#ifdef _1_
  // update end positions
  if (filters.size() > 1)
  {
    for (int i = 0; i < filters.size() - 1; i++)
    {
      filters[i].end_pos = filters[i + 1].start_pos - 2;
    }
  }
  if (filters.size() > 0)
  {
    filters[filters.size() - 1].end_pos = sec.end_pos - 1;
  }
#endif

  return objects;
}

int main(int argc, char *argv[])
{
  cxxopts::Options options("mapalyze", "A tool to analyze map files generated by gcc.");
  options
      .positional_help("[file.map]");
  options.add_options()("f,file", "Name of the map file", cxxopts::value<std::string>())("h,help", "Print usage")("filename", "Name of the map file", cxxopts::value<std::string>());
  options.parse_positional("filename");
  auto result = options.parse(argc, argv);

  if (result.count("help"))
  {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  if (!result.count("filename"))
  {
    std::cout << "ERROR: filename must be specified." << std::endl
              << std::endl;
    ;
    std::cout << options.help() << std::endl;
    exit(-1);
  }

  auto &filename = result["filename"].as<std::string>();

  cout << "Read file..." << endl;
  std::string str = get_file_contents(filename.c_str());

  // find sections
  auto mem_cfg_pos = str.find("Memory Configuration", 0);

  // get the firts entry which is a linker placement
  auto linker_pos = str.find("Linker script and memory map", mem_cfg_pos);
  auto linker_nl_pos = str.find("\n", linker_pos) + 1;
  linker_nl_pos = str.find("\n", linker_nl_pos) + 1;
  auto output_pos = str.find("OUTPUT(", linker_nl_pos);

  cout << "Extract..." << endl;
  auto mem_cfg = str.substr(mem_cfg_pos, linker_pos - mem_cfg_pos);
  auto linkage = str.substr(linker_nl_pos, output_pos - linker_nl_pos);

  cout << "Parse..." << endl;
  auto sections = parse_sections(linkage);
  vector<Filter> filters_combined;
  for (auto s : sections)
  {
    auto filters = parse_filters(linkage, s);
    filters_combined.insert(filters_combined.end(), filters.begin(), filters.end());
  }
  auto fil = filters_combined[filters_combined.size() - 7];

  vector<Object> objects_combined;
  for (auto f : filters_combined)
  {
    auto objects = parse_objects(linkage, f);
    objects_combined.insert(objects_combined.end(), objects.begin(), objects.end());
  }

  for (auto i : objects_combined)
  {
   // cout << i << endl;
  }
  return 0;
}
