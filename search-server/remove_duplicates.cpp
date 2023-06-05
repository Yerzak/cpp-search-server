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
//не та выдача результатов, т.к. идем по id_to_word_freqs_, отсортированному по строкам
//надо использовать GetWordFrequencies
/*Ожидаемый вывод программы:
Before duplicates removed: 9
Found duplicate document id 3
Found duplicate document id 4
Found duplicate document id 5
Found duplicate document id 7
After duplicates removed: 5 */
//почему 6-й документ не удаляется??
//может он криво добавляется??