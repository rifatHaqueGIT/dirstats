/// =========================================================================
/// Written by pfederl@ucalgary.ca in 2020, for CPSC457.
/// =========================================================================
/// You need to edit this file.
///
/// You can delete all contents of this file and start from scratch if
/// you wish, but you need to implement the getDirStats() function as
/// defined in "getDirStats.h".

#include "getDirStats.h"
#include "digester.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static bool
is_dir( const std::string & path)
{
  struct stat buff;
  if( 0 != stat( path.c_str(), & buff))
    return false;
  return S_ISDIR(buff.st_mode);
}

// ======================================================================
// You need to re-implement this function !!!!
// ======================================================================
//
// getDirStats() computes stats about directory dir_name
// if successful, it return true and stores the results in 'res'
// on failure, it returns false, and res is in undefined state
//
bool
getDirStats(const std::string & dir_name, Results & res)
{
  // The results below are all hard-coded, to show you all the fields
  // you need to calculate. You should delete all code below and
  // replace it with your own code.

  if( ! is_dir(dir_name))
    return false;

  // prepare a fake result
  res.largest_file_path = dir_name + "/some_dir/some_file.txt";
  res.largest_file_size = 123;
  res.n_files = 321;
  res.n_dirs = 333;
  res.all_files_size = 1000000;
  res.most_common_types = {
    "C source, ASCII text",
    "makefile script, ASCII text",
    "C++ source, ASCII text",
    "directory"
  };
  std::vector<std::string> group1;
  group1.push_back(dir_name + "/file1.cpp");
  group1.push_back(dir_name + "/lib/sub/other.c");
  res.duplicate_files.push_back(group1);
  std::vector<std::string> group2;
  group2.push_back(dir_name + "/readme.md");
  group2.push_back(dir_name + "/docs/readme.txt");
  group2.push_back(dir_name + "/x.y");
  res.duplicate_files.push_back(group2);

  return true;
}
