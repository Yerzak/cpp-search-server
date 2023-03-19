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
    //При добавлении документа нужно вычислить TF каждого входящего слова. Это нужно делать в методе AddDocument. Можно пройтись циклом по всему документу, увеличивая TF каждого встречаемого слова на величину 1 / N, где N — общее количество слов в документе. Если слово встретилось K раз, то вы столько же раз прибавите к его TF величину 1 / N и получите K / N.
    void AddDocument(int document_id, const string& document) {
        const vector <string> words = SplitIntoWordsNoStop(document);
        ++document_count_;
        double tf = 1.0 / static_cast <double>(words.size());
        for (const string word : words) {
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
    int MinusOrStop(string& word) const {
        if (word[0] == '-') {
            word = word.substr(1);// если минус-слово, отрезаем минус
            if (stop_words_.count(word)) {
                return 1;// если является стопом, следующая итерация
            } else { return 0; 
            }
        }
        return 2;
    }
    /* Парсинг слова запрос будет удобней обрабатывать в вспомогательной функции.
Новая функция должна уже вернуть слово без минуса и информацию про него: является ли оно минус-словом, является ли стоп-словом
А в этом функции останется только решать в какой контейнер поместить.*/
    Query ParseQuery(const string& text) const {
        Query query;
        for (string& word : SplitIntoWordsNoStop(text)) {
            int is_minus_or_stop = MinusOrStop(word);
            if (is_minus_or_stop == 0) { // если минус-слово
                query.minus_words.insert(word);
            } else if (is_minus_or_stop == 1) { // если после удаления минуса получается стоп-слово
                continue;
            } else { // если плюс-слово
                query.plus_words.insert(word);
            }
        }
        return query;// возврат объекта структуры
    }

    double FindIDF(const int count, const int size) const {
        double idf = log(static_cast <double>(count) / static_cast <double>(size));
        return idf;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        map <int, double> document_to_relevance;
        for (const auto& word : query_words.plus_words) {
            if (word_to_document_freqs_.count(word)) {
                double idf = FindIDF(document_count_, word_to_document_freqs_.at(word).size());
                /*C++ справится с задачей инициализировать переменные тогда, когда надо. Думаю, что в этом случае – наоборот – инициализация и переиспользование переменной вредит.
Где используешь – там и создавай.
Так как IDF это конкретная формула, её можно вынести в отдельную функцию, чтобы освободить этот код от нагрузки при чтении кода.*/
                for (const auto [id, tf] : word_to_document_freqs_.at(word)) {
                    document_to_relevance[id] += (tf * idf);
                }
            }
        }
        for (const auto& word : query_words.minus_words) {
            if (word_to_document_freqs_.count(word)) {
                for (const auto [id, tf] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(id);
                }
            }
        }
        vector <Document> result(document_to_relevance.size());
        /*Хороший способ.
Единственное, что не нравится, что отдельно id и relevance записываешь. Можно в одной строчке сделать.
idd relevanced можно не переименовывать, потому что они не конфликтуют с .id и .relevance*/
        int counter = 0;
        for (const auto& [id, relevance] : document_to_relevance) {
            result[counter] = { id, relevance };
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