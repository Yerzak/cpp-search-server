#include "string_processing.h"
#include <algorithm>
std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    int position = 0;
    int length = 0;
    std::for_each(text.begin(), text.end(), [&](const char c) {
        if (c == ' ') {
            if (length!=0) {
                words.push_back(text.substr(position, length));
                position += length;
                length = 0;
            }
            ++position;
        } else {
            ++length;
        }
    }); 
    if (length != 0) {
        words.push_back(text.substr(position, length));
    }
    return words;
}
