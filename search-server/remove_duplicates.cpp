#include "remove_duplicates.h"
#include <set>
#include <iostream>
#include <string>
#include <map>
#include <cstdlib>

void RemoveDuplicates(SearchServer& search_server){
    std::set <int> duplicates;
    std::map <std::set<std::string>, int> for_duplicates;
    for (auto id : search_server) {
        std::set<std::string> words;
        const auto what_ = search_server.GetWordFrequencies(id);
        for (auto [word, fr] : what_) {
            words.insert(word);
        }        
        if (for_duplicates.count(words)) {
            duplicates.insert(std::max(id, for_duplicates.at(words)));
        } else {
            for_duplicates.emplace(words, id);
        }    
    }
    for (const int id : duplicates) {
        std::cout << std::string("Found duplicate document id ") << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
