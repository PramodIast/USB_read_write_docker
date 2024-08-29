#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdexcept>
#include <regex>

using namespace std;

string exec(const char* cmd) {
    char buffer[128];
    string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

string find_usb_partition() {
    string output = exec("lsblk -o NAME,TYPE | grep 'part'");
    cout << "lsblk output:\n" << output << endl;  // Debug output

    istringstream iss(output);
    string line;
    while (getline(iss, line)) {
        cout << "Processing line: " << line << endl;  // Debug output

        // Clean up special characters and extract the partition name
        regex partition_regex("([a-z][a-z0-9]+[0-9]+)\\s+part");
        smatch match;
        if (regex_search(line, match, partition_regex)) {
            string partition = match.str(1);
            cout << "Match found: " << partition << endl;  // Debug output
            return "/dev/" + partition;
        }
    }
    throw runtime_error("No USB partition found.");
}

void create_mount_point(const string& partition_name) {
    string mount_dir = "/mnt/" + partition_name.substr(partition_name.find_last_of('/') + 1);
    struct stat st = {0};
    if (stat(mount_dir.c_str(), &st) == -1) {
        mkdir(mount_dir.c_str(), 0755);
    }
}

bool is_mounted(const string& mount_dir) {
    string mount_output = exec(("mount | grep '" + mount_dir + "'").c_str());
    return !mount_output.empty();
}

void mount_usb(const string& partition, const string& mount_dir) {
    if (is_mounted(mount_dir)) {
        cout << "Partition already mounted at " << mount_dir << endl;
        return;
    }

    string mount_cmd = "mount " + partition + " " + mount_dir;
    cout << "Mount command: " << mount_cmd << endl;  // Debug output
    int result = system(mount_cmd.c_str());
    if (result != 0) {
        throw runtime_error("Failed to mount the USB partition. Ensure you have root privileges.");
    }
}

void write_to_file(const string& file_path) {
    ofstream file(file_path);
    if (!file.is_open()) {
        throw runtime_error("Failed to open file for writing.");
    }
    for (int i = 0; i < 10; ++i) {
        file << "test" << endl;
    }
    file.close();
}

string read_from_file(const string& file_path) {
    ifstream file(file_path);
    if (!file.is_open()) {
        throw runtime_error("Failed to open file for reading.");
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

int main() {
    try {
        if (geteuid() != 0) {
            cerr << "This program requires root privileges. Please run with sudo." << endl;
            return 1;
        }

        string usb_partition = find_usb_partition();
        cout << "Found USB partition: " << usb_partition << endl;

        string partition_name = usb_partition.substr(usb_partition.find_last_of('/') + 1);
        string mount_dir = "/mnt/" + partition_name;
        create_mount_point(partition_name);
        cout << "Mount point created: " << mount_dir << endl;

        mount_usb(usb_partition, mount_dir);
        cout << "USB partition mounted at " << mount_dir << endl;

        string file_path = mount_dir + "/test_file.txt";
        write_to_file(file_path);
        cout << "Wrote to file." << endl;

        sleep(2);  // Ensure file operations are completed

        string contents = read_from_file(file_path);
        cout << "Read from file:" << endl;
        cout << contents << endl;

        string expected_content;
        for (int i = 0; i < 10; ++i) {
            expected_content += "test\n";
        }

        if (contents == expected_content) {
            cout << "File content verified successfully." << endl;
        } else {
            cout << "File content does not match expected output." << endl;
        }

    } catch (const exception& e) {
        cerr << "An error occurred: " << e.what() << endl;
    }

    return 0;
}

