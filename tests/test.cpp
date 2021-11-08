#include <string>
#include <filesystem>
#include <list>
#include <map>
#include <exception>
#include <iostream>
#include <fstream>
#include <utility>

enum ERRORS {
    INSUFFICENT_PARAMS = 1, NO_FILE_IN_RANGE, DIFF_WAS_FOUND
};

using std::string;
using std::list;
using std::map;

class FileNotFoundInRange : public std::exception {
    std::string i;
public:
    explicit FileNotFoundInRange(int idx) : i(std::to_string(idx)) {}

    void message() { std::cout << "File #" << i << " not found!" << std::endl; }
};

class FileDiffExc : public std::exception {
public:
    string file1;
    string file2;
    int line;
    string final_message;

    FileDiffExc(string file1, string file2, int line) : file1(std::move(file1)), file2(std::move(file2)), line(line) {
        final_message =
                "file: '" + file1 + "' and file: '" + file2 + "' ,differ in line: " + std::to_string(line) + "\n";
    }
};


/*
 * e.g. Pcode1200.txt --> 1200
 * else -1
*/
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
    if (argc < 3) {
        std::cout << "A range < X Y > for testing must be given as an argument." << std::endl;
        exit(ERRORS::INSUFFICENT_PARAMS);
    } else if (argc > 3) {
        std::cout << "WARNING: All arguments after 3rd arg are ignored." << std::endl;
    }
    int low_range = std::atoi(argv[1]), high_range = std::atoi(argv[2]);

    string path_to_data = "/mnt/c/Users/Dany/CLionProjects/Compilation/data";
    std::list<string> my_output, orig_output, my_pcode, orig_pcode;

    for (const auto &abs_path: std::filesystem::directory_iterator(path_to_data)) {
        const string &full_abs_path = abs_path.path().string();
        size_t base_index = full_abs_path.rfind('/') + 1;
        const string &base_filename = full_abs_path.substr(base_index);

        /*my pcode*/
        if (StrStartsWith(base_filename, my_pcode_base)) {
            my_pcode.insert(my_pcode.end(), base_filename);
        }
        /*orig pcode*/
        if (StrStartsWith(base_filename, orig_pcode_base)) {
            orig_pcode.insert(orig_pcode.end(), base_filename);
        }
        /*my output*/
        if (StrStartsWith(base_filename, my_output_base)) {
            my_output.insert(my_output.end(), base_filename);
        }
        /*orig output*/
        if (StrStartsWith(base_filename, orig_output_base)) {
            orig_output.insert(orig_output.end(), base_filename);
        }
    }
    my_output.sort();
    orig_output.sort();
    my_pcode.sort();
    orig_pcode.sort();

    map<string, list<string>> dict;
    dict.insert(std::pair<string, list<string>>("my_output", my_output));
    dict.insert(std::pair<string, list<string>>("orig_output", orig_output));
    dict.insert(std::pair<string, list<string>>("my_pcode", my_pcode));
    dict.insert(std::pair<string, list<string>>("orig_pcode", orig_pcode));

    /* Printing processed filenames... */
    for (const auto &begin: dict) {
        std::cout << "Printing '" + begin.first + "' files..." << std::endl;
        for (const auto &path: begin.second) {
            std::cout << path << std::endl;
        }
    }


    auto find_kth_in_cont = [](int k, auto &&it, const auto &&it_end) {
        int i = 1;
        while (i <= k) {
            if (it == it_end) {
                throw FileNotFoundInRange(i);
            }
            if (i < k) {
                i++;
                it++;
            } else
                break;
        }
        return *it;
    };

    namespace fs = std::filesystem;

    /* Compare outputs... */
    std::cout << "\nComparing Pcode in range: <" << std::to_string(low_range) << "," + std::to_string(high_range)
              << ">..."
              << std::endl;
    fs::path base_dir(path_to_data);
    for (int i = low_range; i <= high_range; ++i) {
        fs::path orig_pfile, my_pfile;
        std::cout << "Processing file 'Pcode" << std::to_string(i) << ".txt'..." << std::endl;
        try {
            orig_pfile = fs::path(find_kth_in_cont(i, dict["orig_pcode"].begin(), dict["orig_pcode"].cend()));
        }
        catch (FileNotFoundInRange &e) {
            e.message();
            exit(ERRORS::NO_FILE_IN_RANGE);
        }
        std::cout << "Processing file 'myPcode" << std::to_string(i) << ".txt'..." << std::endl;
        try {
            my_pfile = fs::path(find_kth_in_cont(i, dict["my_pcode"].begin(), dict["my_pcode"].cend()));
        }
        catch (FileNotFoundInRange &e) {
            e.message();
            exit(ERRORS::NO_FILE_IN_RANGE);
        }
        fs::path full_my_pfile_path = base_dir / my_pfile;
    }


    return 0;
}