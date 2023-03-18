#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector <string> words = SplitIntoWordsNoStop(document);
        ++document_count_;
        double tf = 1.0 / static_cast <double>(words.size());

        for (const string word : words)
        {
            word_to_document_freqs_[word][document_id] += tf;

        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;

    struct Query {
        set <string> plus_words;
        set <string> minus_words;
    };

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (string& word : SplitIntoWordsNoStop(text))
        {
            if (word[0] == '-')// если минус-слово
            {
                word = word.substr(1);// типа отрезаем минус
                if (stop_words_.count(word))
                {
                    continue;// если является стопом, следующая итерация
                }
                else {
                    query.minus_words.insert(word);// если не стоп, добавляем в минус-слова
                }
            }
            else {
                query.plus_words.insert(word);

            }// если плюс-слово, добавляем в плюс-слова

        }
        return query;// возврат объекта структуры
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        map <int, double> document_to_relevance;
        double idf;

        for (const auto& word : query_words.plus_words) {
            if (word_to_document_freqs_.count(word)) {
                idf = log(static_cast <double>(document_count_) / static_cast <double>(word_to_document_freqs_.at(word).size()));
                for (const auto [id, tf] : word_to_document_freqs_.at(word))
                {
                    document_to_relevance[id] += (tf * idf);
                }
            }
        }

        for (const auto& word : query_words.minus_words) {
            if (word_to_document_freqs_.count(word))
            {
                for (const auto [id, tf] : word_to_document_freqs_.at(word))
                {
                    document_to_relevance.erase(id);
                }
            }
        }

        vector <Document> result(document_to_relevance.size());
        int counter = 0;
        for (const auto& [idd, relevanced] : document_to_relevance) {
            result[counter].id = idd;
            result[counter].relevance = relevanced;
            ++counter;
        }

        return result;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}
