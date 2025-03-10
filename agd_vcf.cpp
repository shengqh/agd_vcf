#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <map>
#include <chrono>
#include <iomanip>

constexpr size_t BUFFER_SIZE = 2 * 1024 * 1024;  // 2MB buffer

class VCFProcessor {
private:
    char* buffer;
    size_t buffer_size;
    std::unordered_map<std::string, std::string> id_map;
    size_t total_variants;
    std::chrono::steady_clock::time_point start_time;

    bool is_pass_line(const char* line, size_t length) {
        int tab_count = 0;
        size_t pos = 0;
        
        while (pos < length && tab_count < 7) {
            if (line[pos++] == '\t') {
                tab_count++;
                if (tab_count == 6) {
                    return (pos + 4 <= length && 
                            line[pos] == 'P' && 
                            line[pos+1] == 'A' && 
                            line[pos+2] == 'S' && 
                            line[pos+3] == 'S' && 
                            (line[pos+4] == '\t' || line[pos+4] == '\n'));
                }
            }
        }
        return false;
    }

public:
    VCFProcessor(const std::string& id_map_file, size_t total_variants_)
        : buffer(new char[BUFFER_SIZE])
        , buffer_size(BUFFER_SIZE)
        , total_variants(total_variants_)
    {
        std::ifstream map_file(id_map_file);
        if (!map_file.is_open()) {
            throw std::runtime_error("Could not open id_map_file: " + id_map_file);
        }

        std::string map_line;
        std::getline(map_file, map_line);  // Skip header

        while (std::getline(map_file, map_line)) {
            std::istringstream iss(map_line);
            std::string col1, col2;
            if (std::getline(iss, col1, '\t') && std::getline(iss, col2, '\t')) {
                if (col2 == "-") {
                    id_map[col1] = col1 + "_INVALID";
                }else{
                    id_map[col1] = col2;
                }
            }
        }
    }

    ~VCFProcessor() {
        delete[] buffer;
    }

    void process() {
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);
        std::cout.tie(nullptr);
        std::cin.rdbuf()->pubsetbuf(buffer, buffer_size);

        std::string line;
        line.reserve(buffer_size);

        // Copy meta-information lines (##) directly
        while (std::getline(std::cin, line) && line[0] == '#' && line[1] == '#') {
            std::cout << line << '\n';
        }

        // Process the header line (#CHROM)
        if (!line.empty() && line[0] == '#') {
            std::istringstream iss(line);
            std::string field;
            
            // Output first 9 columns unchanged
            for (int i = 0; i < 9; ++i) {
                std::getline(iss, field, '\t');
                if (i > 0) std::cout << '\t';
                std::cout << field;
            }

            // Process and output sample names
            bool first = true;
            while (std::getline(iss, field, '\t')) {
                std::cout << '\t';
                auto it = id_map.find(field);
                std::cout << (it != id_map.end() ? it->second : field);
            }
            std::cout << '\n';
        } else {
            throw std::runtime_error("Invalid VCF format: missing header line");
        }

        start_time = std::chrono::steady_clock::now();

        // Process data lines
        size_t count = 0;
        while (std::cin.getline(buffer, buffer_size)) {
            size_t length = std::cin.gcount() - 1;
            if (is_pass_line(buffer, length)) {
                std::cout.write(buffer, length);
                std::cout.put('\n');
            }
            count++;
            if (count % 10000 == 0 ){
                if( total_variants > 0) {
                    auto now = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                    double fraction = double(count) / double(total_variants);
                    double total_time = fraction > 0 ? elapsed / fraction : 0;
                    double remaining = total_time - elapsed;
                    std::string unit = "Seconds";
                    if (remaining > 60) {
                        remaining /= 60;
                        unit = "Minutes";
                    }
                    if (remaining > 60) {
                        remaining /= 60;
                        unit = "Hours";
                    }
                    auto t = std::time(nullptr);
                    auto tm = *std::localtime(&t);
                    std::cerr << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ") 
                         << count << "/" << total_variants << ": " 
                         << std::fixed << std::setprecision(1) 
                         << fraction * 100.0 << "% processed, ~" 
                         << remaining << " " << unit <<" remaining" << std::endl;
                } else {
                    std::cerr << count << ", " << std::flush;
                }
            }
            if (count % 100000 == 0 && total_variants == 0) {
                std::cerr << std::endl << std::flush;
            }
        }
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::cerr << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ") << count << std::endl << std::flush;
    }
};

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
  const char* VERSION = "1.0.1";

  if (argc > 1 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version")) {
    std::cout << "Version: " << VERSION << std::endl;
    return 0;
  }

  auto args = parse_args(argc, argv);
  if (args.find("--id_map_file") == args.end() || args.find("--total_variants") == args.end()) {
    std::cerr << "Usage: " << argv[0] << " --id_map_file=<file> --total_variants=<num>" << std::endl;
    std::cerr << "Version: " << VERSION << std::endl;
    return 1;
  }

  try {
    VCFProcessor processor(args["--id_map_file"], std::stoul(args["--total_variants"]));
    processor.process();
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}