#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <map>
#include <zlib.h>

void filter_vcf(const std::string& id_map_file, gzFile gz_out) {
    // Read the id_map_file and store the mappings
    std::unordered_map<std::string, std::string> id_map;
    std::ifstream map_file(id_map_file);
    if (!map_file.is_open()) {
        std::cerr << "Could not open id_map_file: " << id_map_file << std::endl;
        return;
    }

    // Ignore the header line
    std::string map_line;
    std::getline(map_file, map_line);

    // Read the rest of the file
    while (std::getline(map_file, map_line)) {
        std::istringstream iss(map_line);
        std::string col1, col2, col3;
        if (std::getline(iss, col1, ',') && std::getline(iss, col2, ',') && std::getline(iss, col3, ',')) {
            if (col3 != "-") {
                id_map[col2] = col3;
            }
        }
    }
    map_file.close();

    // Process the VCF file from standard input
    std::string line;
    bool header_processed = false;

    // First section: print lines starting with "##"
    while (std::getline(std::cin, line)) {
        if (line.substr(0, 2) == "##") {
            if (gz_out) {
                gzputs(gz_out, (line + "\n").c_str());
            } else {
                std::cout << line << std::endl;
            }
        } else if (line.substr(0, 6) == "#CHROM") {
            // Process the header line with sample names
            std::istringstream iss(line);
            std::vector<std::string> columns;
            std::string column;
            while (std::getline(iss, column, '\t')) {
                columns.push_back(column);
            }

            // Replace sample names in the header line
            for (size_t i = 9; i < columns.size(); ++i) {
                if (id_map.find(columns[i]) != id_map.end()) {
                    columns[i] = id_map[columns[i]];
                }
            }

            // Print the modified header line
            std::ostringstream oss;
            for (size_t i = 0; i < columns.size(); ++i) {
                if (i > 0) oss << '\t';
                oss << columns[i];
            }
            oss << "\n";
            if (gz_out) {
                gzputs(gz_out, oss.str().c_str());
            } else {
                std::cout << oss.str();
            }
            header_processed = true;
            break;
        } else {
            throw std::runtime_error("Unexpected line format: " + line);
        }
    }

    // Second section: process data lines
    if (header_processed) {
        while (std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::vector<std::string> columns;
            std::string column;
            for (int i = 0; i < 7 && std::getline(iss, column, '\t'); ++i) {
                columns.push_back(column);
            }

            if (columns.size() == 7 && columns[6] == "PASS") {
                if (gz_out) {
                    gzputs(gz_out, (line + "\n").c_str());
                } else {
                    std::cout << line << std::endl;
                }
            }
        }
    }
}

std::map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t pos = arg.find('=');
        if (pos != std::string::npos) {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
            args[key] = value;
        }
    }
    return args;
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);
    if (args.find("--id_map_file") == args.end()) {
        std::cerr << "Usage: " << argv[0] << " --id_map_file=<id_map_file> [-o=<output_gz_file>]" << std::endl;
        return 1;
    }

    std::string id_map_file = args["--id_map_file"];
    gzFile gz_out = nullptr;
    if (args.find("-o") != args.end()) {
        gz_out = gzopen(args["-o"].c_str(), "wb");
        if (!gz_out) {
            std::cerr << "Could not open output file: " << args["-o"] << std::endl;
            return 1;
        }
    }

    try {
        filter_vcf(id_map_file, gz_out);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (gz_out) gzclose(gz_out);
        return 1;
    }

    if (gz_out) gzclose(gz_out);
    return 0;
}

