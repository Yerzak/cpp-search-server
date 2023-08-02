#pragma once
#include "search_server.h"
#include <string>
#include <deque>
#include "document.h"
#include <vector>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
    : search_server_(search_server) {
        // напишите реализацию
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        QueryResult q;
        ++time_counter;
        q.time = time_counter;
        if ((q.time - requests_.front().time) >= min_in_day_) {
            QueryResult first = requests_.front();
            if (first.IsEmpty) {
                requests_.pop_front();
                empty_requests_.pop_front();
            }
            else {
                requests_.pop_front();
            }
        }
        auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (!result.empty()) {
            q.IsEmpty = false;
            requests_.push_back(q);
        } else {
            q.IsEmpty = true;
            requests_.push_back(q);
            empty_requests_.push_back(q);
        }
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool IsEmpty;
        int time; // определите, что должно быть в структуре
    };
    std::deque<QueryResult> requests_;
    std::deque<QueryResult> empty_requests_;
    const static int min_in_day_ = 1440;
    int time_counter=0;
    const SearchServer& search_server_;
    // возможно, здесь вам понадобится что-то ещё
};
