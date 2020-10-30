#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <cxxopts.hpp>
#include <string>
#include <fstream>
#include <streambuf>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;

struct MapSection
{
  uint32_t begin_adr;
  uint32_t end_adr;
  std::string object_name;

  uint32_t GetSize() const
  {
    return end_adr - begin_adr + 1;
  }

  friend ostream &operator<<(ostream &os, const MapSection &msec);
};

ostream &operator<<(ostream &os, const MapSection &msec)
{
  os << "0x" << hex << uppercase << setw(8) << msec.begin_adr << " |"
     << "0x" << hex << uppercase << setw(8) << msec.end_adr
     << " |" << dec << setw(8) << msec.GetSize() << " | " << msec.object_name;
  return os;
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

std::vector<std::string>
split(std::string const &original, char separator)
{
  std::vector<std::string> results;
  std::string::const_iterator start = original.begin();
  std::string::const_iterator end = original.end();
  std::string::const_iterator next = std::find(start, end, separator);
  while (next != end)
  {
    results.push_back(std::string(start, next));
    start = next + 1;
    auto next_char = start + 1;
    next = std::find(start, end, separator);

    while (next == next_char)
    {
      start = next + 1;
      next_char = start + 1;
      next = std::find(start, end, separator);
    }
  }
  results.push_back(std::string(start, next));
  return results;
}
bool is_space(char c)
{
  return c == ' ';
}

MapSection parse_entry(string const &entry)
{
  vector<string> container;
  //container.reserve(5);

  auto splitted = boost::split(container, entry, is_space, boost::token_compress_on);
  if (splitted.size() <= 3)
  {
    throw "No enough elements";
  }

  if (!boost::starts_with(splitted[1], "0x"))
  {
    throw "First element is not a hex value";
  };

  std::size_t processed = 0;
  auto address = std::stoul(splitted[1].substr(2, string::npos), &processed, 16);

  if (!boost::starts_with(splitted[2], "0x"))
  {
    throw "Second element is not a hex value";
  };

  auto size = std::stoul(splitted[2].substr(2, string::npos), &processed, 16);

  MapSection sec = {
      .begin_adr = static_cast<uint32_t>(address),
      .end_adr = static_cast<uint32_t>(address) + static_cast<uint32_t>(size) - 1,
      .object_name = splitted[3]};

  return sec;
}

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

Section parse_section(const string &line, unsigned start_pos)
{

  vector<string> container;
  //container.reserve(5);

  auto splitted = boost::split(container, line, is_space, boost::token_compress_on);

  Section new_sec = {
      .section_name = splitted[0],
      .adress = splitted[1],
      .size = splitted[2].substr(0, splitted[2].size() - 1),
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

  vector<MapSection> list;
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
    sections[i].end_pos = sections[i + 1].start_pos - 1;
  }
  sections[sections.size()-1].end_pos = str.size()-1;
  return sections;
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
  for (auto s : sections)
  {
    cout << s << endl;
  }

  auto last = sections[sections.size()-1];
  cout<<linkage.substr(last.start_pos, last.end_pos-last.start_pos) << ".." << endl;

  exit(0);
#ifdef _1_
  cout << "Iterate..." << endl;
  auto it = sections.begin();
  auto last = *it;
  it++; // iterate from second element
  for (; it != sections.end(); it++)
  {
  }

  sort(sections.begin(), sections.end(), [](MapSection a, MapSection b) {
    return (a.begin_adr > b.begin_adr);
  });

  for (auto v : sections)
  {
    cout << v << endl;
  }
#endif
  //auto contents = split(str, '\n');
  //std::cout << contents.size() << std::endl;
  return 0;
}
