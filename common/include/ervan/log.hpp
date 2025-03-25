#pragma once

#include <ctime>
#include <iostream>

namespace ervan::log {
    class log_output {
        public:
        log_output();
        log_output(std::ostream& output) : _output(output) {}

        void set_name(const char* name) {
            this->_name = name;
        }

        log_output& operator<<(std::string line) {
            char time_text[32];
            this->_output << preambule(time_text, sizeof(time_text)) << " " << this->_name << ": "
                          << line << std::endl;

            return *this;
        }

        private:
        std::ostream& _output;
        const char*   _name;

        char* preambule(char* buffer, size_t len) {
            std::time_t t = std::time(nullptr);
            strftime(buffer, len, "%H:%M:%S", std::localtime(&t));
            return buffer;
        }
    };

    extern log_output out;
}