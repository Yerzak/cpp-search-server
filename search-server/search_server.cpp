#include <numeric>
#include "search_server.h"

std::set <int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set <int>::iterator SearchServer::end() {
    return document_ids_.end();
}
void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument(std::string("Invalid document_id"));
    }

    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument(std::string("Document with your id has exist yet"));
    }
    const auto words = SplitIntoWordsNoStop(document);
    std::set <std::string, std::less<>> text_;
    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        auto push_it = text_.insert(std::string(word));
        word_to_document_freqs_[static_cast <std::string_view> (*push_it.first)][document_id] += inv_word_count;
        id_to_word_freqs_[document_id][static_cast <std::string_view> (*push_it.first)] += inv_word_count;
        //здесь нужно добавлять документы в id_to_words_freqs, вычисляя частоту слов документа в этом документе
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::move(text_) });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status = DocumentStatus::ACTUAL, int rating) {
            return document_status == status;
        });
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

SearchServer::MatchResult SearchServer::MatchDocument(const std::string_view raw_query,
    int document_id) const {//LOG_DURATION_STREAM(std::string("Operation time"), std::cout);
    const auto query = ParseQuery(raw_query);
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](const auto word) {
        return (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id));
        })) {
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        if (!word_to_document_freqs_.count(word)) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

SearchServer::MatchResult  SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query,
    int document_id) const {
    return MatchDocument(raw_query, document_id);
}

SearchServer::MatchResult SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query,
    int document_id) const {
    auto query = ParseQueryPar(raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());
    //сначала проверка на минус-слова
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](auto word) {
        return (documents_.count(document_id) && documents_.at(document_id).text.count(word));//проверка на наличие слов должна быть организована в другом хранилище - documents_.at(document_id).text.count(word)
        })) {
        return { {}, documents_.at(document_id).status };
    }
    //проверка на плюс-слова documents_.at(document_id).text.count(word)    
    auto words_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](auto word) {
        return (documents_.count(document_id) && documents_.at(document_id).text.count(word));
        });
    std::sort(matched_words.begin(), words_end);
    auto it_end_second = std::unique(matched_words.begin(), words_end);
    matched_words.erase(it_end_second, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map <std::string_view, double> result;
    if (id_to_word_freqs_.count(document_id)) {
        result = id_to_word_freqs_.at(document_id);
    }
    return result;
}
void SearchServer::RemoveDocument(int document_id) {
    if (!id_to_word_freqs_.count(document_id)) {
        return;
    }
    std::vector <std::string_view> container;
    for (auto words : id_to_word_freqs_.at(document_id)) {
        container.push_back(words.first);
    }
    for (auto word : container) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    documents_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (!id_to_word_freqs_.count(document_id)) { return; }
    auto& word_freqs = id_to_word_freqs_.at(document_id);
    std::vector <const std::string_view*> new_vec(word_freqs.size());//создаем вектор указателей для слов
    std::transform(std::execution::par, word_freqs.begin(), word_freqs.end(), new_vec.begin(),
        [&](const auto& adress) {
            return &adress.first;
        });//закидываем в этот вектор адреса всех слов базы документов
    std::for_each(std::execution::par, new_vec.begin(), new_vec.end(), [&](const auto& adress) {//Используем for_each для этого вектора и удаляем документы из нужной мапы
        word_to_document_freqs_[*adress].erase(document_id);
        });
    id_to_word_freqs_.erase(document_id);//Из других мап удаляем с помощью erase
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument(std::string("Word ") + std::string(word) + std::string(" is invalid"));
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    rating_sum = std::accumulate(ratings.begin(), ratings.end(), rating_sum);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument(std::string("Query word is empty"));
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument(std::string("Query word ") + std::string(text) + std::string(" is invalid"));
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    std::sort(result.plus_words.begin(), result.plus_words.end(), [](auto prev, auto post) {
        return prev < post;
        });
    auto it_end = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(it_end, result.plus_words.end());
    std::sort(result.minus_words.begin(), result.minus_words.end(), [](auto prev, auto post) {
        return prev < post;
        });
    it_end = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(it_end, result.minus_words.end());
    return result;
}

SearchServer::Query SearchServer::ParseQueryPar(const std::string_view text) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
