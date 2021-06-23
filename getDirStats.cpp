#include "getDirStats.h"
#include "digester.h"
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <vector>
#include <stdio.h>
#include <unordered_map>
/*
*Function provided by Pavol Federl 
*/
static bool
is_dir(const std::string & path)
{
  struct stat buff;
  if (0 != stat(path.c_str(), & buff)) return false;
  return S_ISDIR(buff.st_mode);
}
/*
* is_file is similar inspired by the method is_dir.
* Checks if the file is a directory, if it is it will add it to the all_file_size. 
* If the file being checked is of a larger size, then the res attributes largest_file_path and largest_file_size
* will be changed to the file being checked.
*/
static bool
is_file(const std::string & path , Results & r)
{
  struct stat buff;

  if (0 != stat(path.c_str(), & buff)) return false;

  if(S_ISREG(buff.st_mode)){

    r.all_files_size += buff.st_size;//Increasing all files size

    if(r.largest_file_size == 0 || r.largest_file_size < buff.st_size)//Comparing largest file with current file
    {
      r.largest_file_path = path;//Set Largest file path to current file path
      r.largest_file_size = buff.st_size;//Changes size of largest file to the current files size
    }
  }
   
  return S_ISREG(buff.st_mode);//return true if the mode is a file
}

constexpr int MAX_WORD_SIZE = 1024;//max word is semi-useless for this program as only words >1024 characters will be tested
bool ting = false;//a global flag to stop the while loop in the function call. Set to true when EOF is detected
/*
*Inspired by Pavol Federl. It follows a similar structure to https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp.
*This function goes through a file and returns a word at a time. This function is very similar to the example provided,
*but this function does not take in 2 or 1 letter words i.e. a, ab, ba1, ca3.
*In addition, this program takes in a passby reference of the file to open
*/
std::string
next_word(FILE *& fp) //NOTATION FOR PASSBY REFRENCE OF FILES is different
{
  std::string result;
  ting = false;// set to false at start of function
  while(1) {
    int c = fgetc(fp); // Change to reading from a buffer 
    if(c == EOF){ 
      if(result.length() < 3)
        result = "";
      ting = true;
      break;
    }
    c = tolower(c);
    if(! isalpha(c)) {
      if (result.length() < 3){ // Words are 3 letters or more
        result = ""; // reset the string
      }
      else if(result.size() == 0)
        continue; 
      else
        break;
    }
    else {
      if(result.size() >= MAX_WORD_SIZE) { // Words are supposed to have >1024 characters
        printf("input exceeded %d word size, aborting...\n", MAX_WORD_SIZE);
        exit(-1);
      }
      result.push_back(c);
    }
  }
  
  return result;
}

//
// getDirStats() computes stats about directory a directory
//   dir_name = name of the directory to examine
//   n = how many top words/filet types/groups to report
//
// if successful, results.valid = true
// on failure, results.valid = false
//
Results 
getDirStats(const std::string & dir_name, int n)
{
 
  Results res;
  res.valid = false;

  res.largest_file_size = -1; // l_f_s is set to 41 for some reason, setting it to zero

  res.largest_file_path = "";

  res.n_dirs = 0;// The n_dirs starts at 121 for some reason, setting it to zero to start

  std::unordered_map<std::string,int> fileTypeHist;//unordered map for file types

  std::unordered_map<std::string,int> commonWordsHist; // unordered map for common words

  std::unordered_map<std::string,std::vector<std::string>> shaMap; //used to hold all the files and their similar hashes

  if (! is_dir(dir_name)) return res; //If the supplied name is not a directory return with no fields

  std::vector<std::string> stack;

  stack.push_back(dir_name); //First directory is the one supplied from commandline

  while( ! stack.empty()) { //while there is still paths to traverse

    auto dirname = stack.back();
    
    stack.pop_back();

    //printf("%s\n", dirname.c_str());

    DIR * dir = opendir( dirname.c_str());

    if(is_dir(dirname)) //check if the path is a directory
      res.n_dirs++; //increase directory cound

    if(is_file(dirname, res) ){ //check if path is a file

    res.n_files++;//Increase file count
   //FileType Part:
    std::string cmd = "file -b " + dirname; // making the command

    FILE * fp = popen( cmd.c_str(), "r");// calling command

    if( fp == nullptr) { // null check
      printf("popen() failed, quitting\n");
      res.valid = false;

      res.largest_file_size = -1; // l_f_s is set to 41 for some reason, setting it to zero

      res.largest_file_path = "";

      exit(-1);
    }

    char buffer[4096]; // buffer for fgets
    
    char * response = fgets(buffer, sizeof(buffer), fp); // putting file type into buffer

     if( response != nullptr) { //checking if fgets is successful
      // find the end of the first field ('\0', or '\n' or ',')
      int eol = 0;
      while(buffer[eol] != ',' && buffer[eol] != '\n' && buffer[eol] != 0) eol ++;
      // terminate the string
      buffer[eol] = 0;
      // remember the type
     fileTypeHist[buffer]++; //Adding the string and incrementing it ( only increments if the value already exists)
    } 
    pclose(fp);
    //End of FileType Part.

    //Start of Common_Word Part:

    FILE * filepath = fopen(dirname.c_str() , "r");

    if( filepath == nullptr) { // null check
      printf("popen() failed, quitting\n");
      res.valid = false;

      res.largest_file_size = -1; // l_f_s is set to 41 for some reason, setting it to zero

      res.largest_file_path = "";

      exit(-1);
    }
    while(1){
        std::string word = next_word(filepath);
      if(!word.empty())//
        commonWordsHist[word]++; //Increment the word in unordered map
      if(ting) break;
    }

    pclose(filepath);
    //End of common Word Part.

    // Start of Hash Part:
      shaMap[sha256_from_file(dirname)].push_back(dirname);
    // End of Hash Part.
    }

    //This part goes throught the directories and is provided by Pavol Federl
    if( dir) {
      while(1) {
	      dirent * de = readdir( dir);
      	if( ! de) break;
      	std::string name = de-> d_name;
      	if( name == "." || name == "..") continue;
	      std::string path = dirname + "/" + de-> d_name;
	      stack.push_back( path);
      }
      closedir( dir);
    }
  }//end of while loop

  // All sorting algorithms are inspired by Pavol Federl. The file used was specific https://gitlab.com/cpsc457/public/word-histogram/-/blob/master/main.cpp .
  //Sorting Hashes Part:
  std::vector<std::pair<int,std::vector<std::string>>> temp3;
  for(auto & h : shaMap) 
    temp3.emplace_back(-h.second.size(), h.second);
  if(temp3.size() > size_t(n)) {
    std::partial_sort(temp3.begin(), temp3.begin() + n, temp3.end());
    // // drop all entries after the first n
    temp3.resize(n);
  } else {
    std::sort(temp3.begin(), temp3.end());
  }
  for(auto & h : temp3){
    if(-h.first > 1) // Negative b/c we did the negative trick
      res.duplicate_files.push_back(h.second);
  }
  //End of Sorting Hashes
  //Sorting FileType Part:
  //Making a temp vector pair to sort
  std::vector<std::pair<int,std::string>> temp;
  for(auto & h : fileTypeHist)
    temp.emplace_back(-h.second, h.first);
  // if we have more than N entries, we'll sort partially, since
  // we only need the first N to be sorted
  if(temp.size() > size_t(n)) {
    std::partial_sort(temp.begin(), temp.begin() + n, temp.end());
    // // drop all entries after the first n
    temp.resize(n);
  } else {
    std::sort(temp.begin(), temp.end());
  }

  //now put it into the actual response
  for(auto & h : temp)
   res.most_common_types.emplace_back(h.second, -h.first);
  //End of Sorting File Part.
  //Sorting Common Word Part:
  std::vector<std::pair<int,std::string>> temp2;//another temporary vector to sort the common words entries
  for(auto & h : commonWordsHist)
    temp2.emplace_back(-h.second, h.first);
  // if we have more than N entries, we'll sort partially, since
  // we only need the first N to be sorted
  if(temp2.size() > size_t(n)) {
    std::partial_sort(temp2.begin(), temp2.begin() + n, temp2.end());
    //drop all entries after the first n
    temp2.resize(n);
  } else {
    std::sort(temp2.begin(), temp2.end());
  }
  for(auto & h : temp2)
   res.most_common_words.emplace_back(h.second, -h.first);
  //End of sorting Common Word Part.
  res.n_dirs -= 1; //removing the top directory in the directory count
  res.valid = true;
  return res;
}
