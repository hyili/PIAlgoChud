#include <iostream>
#include <string>
#include <unordered_map>

#include <signal.h>
#include "chudnovsky.hpp"

using namespace std;

int ParseParameters(unordered_map<string, string>& config, int argc, char** argv) {
    config["mode"] = "m";
    config["worker"] = "-1";
    config["version"] = "2";

    for (int i = 1; i < argc; i++) {
        string para = argv[i];
        if (para == "-p") {
            ++i;
            if (i >= argc) cerr << " [X] Please give a number for digits of PI after -p" << endl;
            config["digits"] = argv[i];
        } else if (para == "-s") {
            config["mode"] = "s";
        } else if (para == "-m") {
            config["mode"] = "m";
        } else if (para == "-sm") {
            config["mode"] = "sm";
        } else if (para == "-h") {
            config["help"] = "set";
        } else if (para == "-n") {
            config["nout"] = "set";
        } else if (para == "-v") {
            ++i;
            if (i >= argc) cerr << " [X] Please give a number for version of multithread implementation after -v" << endl;
            config["version"] = argv[i];
        } else if (para == "-w") {
            ++i;
            if (i >= argc) cerr << " [X] Please give a number for digits of PI after -p" << endl;
            config["worker"] = argv[i];
        } else {
            cerr << " [X] What is this? (" << para << ")" << endl;
        }
    }

    if (config.find("digits") == config.end() || config.find("help") != config.end()) {
        cerr << "usage: {exe} -p {digits} [-w {workers}] [-v {version}] [(-s|-m|-sm)] [(-n)]" << endl;
        cerr << endl;
        cerr << "   -p: specify the precision of PI." << endl;
        cerr << "   -w: specify the number of worker." << endl;
        cerr << "   -v: specify the verion of multithread implementation. Currently 1, 2, 3 is available, and default is 2." << endl;
        cerr << "   -s: using single thread mode to calculate PI." << endl;
        cerr << "   -m: using multi thread mode to calculate PI. Default." << endl;
        cerr << "   -sm: using both single thread and multi thread mode to calculate PI." << endl;
        cerr << "   -n: do not output." << endl;
        cerr << "   -h: print this message." << endl;
        return -1;
    }

    return 0;
}

int main(int argc, char** argv) {
    unordered_map<string, string> config;
    if (ParseParameters(config, argc, argv) == -1) {
        return -1;
    }

    try {
        // instantiation
        Chudnovsky calc(stoi(config["version"]), stoi(config["digits"]), stoi(config["worker"]));

        // single thread
        if (config["mode"].find("s") != string::npos) {
            cerr << " [*] Single Thread Mode: " << endl;
            calc.Start(config.find("nout") != config.end());
        }

        // for concurrency
        if (config["mode"].find("m") != string::npos) {
            cerr << " [*] Multi Thread Mode: " << endl;
            calc.StartConcurrent(config.find("nout") != config.end());
        }
    } catch (...) {
        cout << " [X] ERROR!" << endl;
        return -1;
    }

    cerr << " [*] Done!" << endl;

    return 0;
}
