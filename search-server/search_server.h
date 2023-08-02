#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <iterator>
#include <execution>
#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public: 
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument(std::string("Some of stop words are invalid"));
        }
    }
    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }
    
    explicit SearchServer(const std::string_view stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    std::set <int>::iterator begin() ; 
    std::set <int>::iterator end() ; 
    
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
    
    template <typename DocumentPredicate>
std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    //LOG_DURATION_STREAM(std::string("Operation time"), std::cout);
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            auto minimal_difference = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < minimal_difference) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> FindTopDocuments(const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    //LOG_DURATION_STREAM(std::string("Operation time"), std::cout);
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);
    std::sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            auto minimal_difference = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < minimal_difference) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}
    
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    
    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query,
    int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query,
    int document_id) const;    
                            
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::set <std::string, std::less<>> text;
    };
    std::map <int, std::map <std::string_view, double>> id_to_word_freqs_;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    
    bool IsStopWord(const std::string_view word) const;
    static bool IsValidWord(const std::string_view word);
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
   
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    QueryWord ParseQueryWord(const std::string_view text) const;
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    Query ParseQuery(const std::string_view text) const;
    Query ParseQueryPar(const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query,
    DocumentPredicate document_predicate) const {
        size_t bucket_count = 100;
    ConcurrentMap <int, double> document_to_relevance(bucket_count);
    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&](const std::string_view word) {
        if (word_to_document_freqs_.count(word)) {
            
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        std::for_each(std::execution::par, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](const auto id_freq) {
            const auto& document_data = documents_.at(id_freq.first);
            if (document_predicate(id_freq.first, document_data.status, document_data.rating)) {
                document_to_relevance[id_freq.first].ref_to_value += id_freq.second * inverse_document_freq;//через Access???
            }
            });}
        });
    auto doc_to_rel = std::move(document_to_relevance.BuildOrdinaryMap());
    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view word) {
        if (word_to_document_freqs_.count(word)) {

        std::for_each(std::execution::par, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](const auto id_smthng) {
            doc_to_rel.erase(id_smthng.first);
            });}
        });
    std::vector<Document> matched_documents;
    std::for_each(std::execution::par, doc_to_rel.begin(), doc_to_rel.end(), [&](const auto id_rel) {
        matched_documents.push_back(
            { id_rel.first, id_rel.second, documents_.at(id_rel.first).rating });
        });
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (!word_to_document_freqs_.count(word)) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view word : query.minus_words) {
        if (!word_to_document_freqs_.count(word)) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}
};