#include <string>
#include <filesystem>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <iostream>
#include <fstream>
#include <utility>
#include <regex>

enum ERRORS {
    INSUFFICENT_PARAMS = 1, NO_FILE_IN_RANGE, DIFF_WAS_FOUND
};

using std::list;
using std::map;
using std::string;
using std::vector;
#define BASE_VEC_SIZE 25

class FileNotFoundInRange : public std::exception {
    std::string i;
public:
    explicit FileNotFoundInRange(int idx) : i(std::to_string(idx)) {}

    void message() { std::cout << "File #" << i << " not found!" << std::endl; }
};

const string &my_pcode_base = "myPcode";
const string &my_output_base = "myoutput";
const string &orig_pcode_base = "Pcode";
const string &orig_output_base = "output";

bool CompareFiles(const string &abs_f1_path, const string &abs_f2_path) {
    return false;
}

bool StrStartsWith(const string &to_search, const string &to_look_for) {
    return to_search.rfind(to_look_for, 0) == 0;
}

/*
 * e.g. Pcode1200.txt --> 1200
 * else -1
*/
int FindSubStrNum(const string &s) {
    size_t start_index = s.find_first_of("0123456789");
    if (start_index == string::npos)
        return -1;
    size_t end_index = s.rfind('.');
    string temp;
    for (size_t i = start_index; i < end_index; i++) {
        temp += s[i];
    }
    return std::atoi(temp.c_str());
}

int main(int argc, char *argv[]) {
    bool has_passed = true, missing_files = false;
    if (argc < 3) {
        std::cout << "A range < X Y > for testing must be given as an argument." << std::endl;
        exit(ERRORS::INSUFFICENT_PARAMS);
    } else if (argc > 3) {
        std::cout << "WARNING: All arguments after 3rd arg are ignored." << std::endl;
    }
    int low_range = std::atoi(argv[1]), high_range = std::atoi(argv[2]);

    string path_to_data = "/mnt/c/Users/Dany/CLionProjects/Compilation/data";
    std::vector<string> my_output(BASE_VEC_SIZE, ""), orig_output(BASE_VEC_SIZE, ""), my_pcode(BASE_VEC_SIZE,
                                                                                               ""), orig_pcode(
            BASE_VEC_SIZE, "");

    for (const auto &abs_path: std::filesystem::directory_iterator(path_to_data)) {
        const string &full_abs_path = abs_path.path().string();
        size_t base_index = full_abs_path.rfind('/') + 1;
        const string &base_filename = full_abs_path.substr(base_index);

        /*my pcode*/
        if (StrStartsWith(base_filename, my_pcode_base)) {
            my_pcode.at(FindSubStrNum(base_filename)) = base_filename;
        }
        /*orig pcode*/
        if (StrStartsWith(base_filename, orig_pcode_base)) {
            orig_pcode.at(FindSubStrNum(base_filename)) = base_filename;
        }
        /*my output*/
        if (StrStartsWith(base_filename, my_output_base)) {
            my_output.at(FindSubStrNum(base_filename)) = base_filename;
        }
        /*orig output*/
        if (StrStartsWith(base_filename, orig_output_base)) {
            orig_output.at(FindSubStrNum(base_filename)) = base_filename;
        }
    }

    map<string, vector<string>> dict;
    dict.insert(std::pair<string, vector<string>>("my_output", my_output));
    dict.insert(std::pair<string, vector<string>>("orig_output", orig_output));
    dict.insert(std::pair<string, vector<string>>("my_pcode", my_pcode));
    dict.insert(std::pair<string, vector<string>>("orig_pcode", orig_pcode));

    /* Printing processed filenames... *//*
    for (const auto &begin: dict) {
        std::cout << "Printing '" + begin.first + "' files..." << std::endl;
        for (const auto &path: begin.second) {
            if (path != "")
                std::cout << path << std::endl;
        }
    }*/

    namespace fs = std::filesystem;

    /* Compare outputs... */
    std::cout << "\nComparing Pcode in range: <" << std::to_string(low_range) << "," + std::to_string(high_range)
              << ">..." << std::endl << std::endl;
    fs::path base_dir(path_to_data);
    list<int> failed_tests;
    for (int i = low_range; i <= high_range; ++i) {
        fs::path orig_pfile, my_pfile;
        std::cout << "Processing file 'Pcode" << std::to_string(i) << ".txt'..." << std::endl;
        try {
            orig_pfile = fs::path(dict["orig_pcode"].at(i));
        }
        catch (std::out_of_range &e) {
            e.what();
            exit(ERRORS::NO_FILE_IN_RANGE);
        }
        fs::path orig_pfile_fullpath = base_dir / orig_pfile;

        std::cout << "Processing file 'myPcode" << std::to_string(i) << ".txt'..." << std::endl;
        try {
            my_pfile = fs::path(dict["my_pcode"].at(i));
        }
        catch (std::out_of_range &e) {
            e.what();
            exit(ERRORS::NO_FILE_IN_RANGE);
        }
        fs::path my_pfile_fullpath = base_dir / my_pfile;

        if ((orig_pfile.string() == "" && my_pfile.string() != "") ||
            ((orig_pfile.string() != "" && my_pfile.string() == ""))) {
            std::cout << "File #" << i << " not found!" << std::endl << std::endl;
            missing_files = true;
            continue;
        }

        std::cout << "Comparing files..." << std::endl << std::endl;
        std::ifstream pcode_stream(orig_pfile_fullpath, std::ifstream::in);
        std::ifstream mypcode_stream(my_pfile_fullpath, std::ifstream::in);
        if (pcode_stream && mypcode_stream) {
            int line = 1;
            std::string line1, line2;
            while (std::getline(pcode_stream, line1) && std::getline(mypcode_stream, line2)) {
                if (line1 != line2) {
                    /*
                     * linux compilers are sensitive snowflakes
                     * if you are on windows remove following lines
                     */
                    uint last1 = line1.length() - 1;
                    uint last2 = line2.length() - 1;
                    if (line1[last1] == '\r') {
                        line1[last1] = '\0';
                    }
                    if (line2[last2] == '\r')
                        line2[last2] = '\0';
                    std::cout << "Line " << line << " does not match!" << std::endl;
                    std::cout << "Difference:" << std::endl;
                    std::cout << "Original pcode: '" << line1 << "'" << std::endl;
                    std::cout << "My pcode: '" << line2 << "'" << std::endl;
                    std::cout << "Test: " << i << " failed." << std::endl << std::endl;
                    failed_tests.emplace_back(i);
                    has_passed = false;
                    break;
                }
                line++;
            }
        }
        pcode_stream.close();
        mypcode_stream.close();
    }
    if (missing_files)
        std::cout << "Warning: Some files in range not found!" << std::endl;
    if (has_passed)
        std::cout << "All compared files passed :)" << std::endl;
    else {
        std::cout << "Tests: ";
        for (const auto &failedTest: failed_tests) {
            std::cout << failedTest << " ";
        }
        std::cout << "have failed :(" << std::endl;
    }

    return 0;
}