//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎
#include "request_queue.h"


    vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        // напишите реализацию
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        // напишите реализацию
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }
    int RequestQueue::GetNoResultRequests() const {
        // напишите реализацию
        return empty_requests_.size();
    }
