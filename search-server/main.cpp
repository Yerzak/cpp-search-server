#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include "search_server.h"
#include <iostream>
#include <sstream>
using namespace std;


// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
void TestExcludeMinusWordsFromAddedDocumentContent() {
    //создаем первый документ со словом, которое будет минусом
    SearchServer server;
    server.AddDocument(0, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    //создаем второй документ без слова, которое будет минусом. Оба должны содержать плюсовое слово
    const auto found_docs = server.FindTopDocuments("white -cat"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 1);
    //вызываем поиск с минус-словом и смотрим результат - должен найти только один документ, в котором не упоминается слово, забракованное нами
}

void TestMatchingDocuments() {
    SearchServer server;
    server.AddDocument(0, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    auto [found_words, status] = server.MatchDocument("white dog -cat"s, 0);

    ASSERT(found_words.size() == 0);

    auto [new_found_words, new_status] = server.MatchDocument("white dog -cat"s, 1);
    ASSERT_EQUAL(new_found_words.size(), 2u);
    ASSERT_EQUAL(new_found_words[0], "dog"s);
    ASSERT_EQUAL(new_found_words[1], "white"s);
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestRelevanceRatingOrder() {
    SearchServer server;
    server.AddDocument(0, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, 4 });
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "black cat small tail loud voice"s, DocumentStatus::ACTUAL, { 14, 4, 6 });
    server.AddDocument(3, "red bird loud voice"s, DocumentStatus::ACTUAL, { 5, 16, 1 });
    server.AddDocument(4, "black big cat with brown small eyes"s, DocumentStatus::ACTUAL, { 1, 2, 9 });
    server.AddDocument(5, "black dog white tail"s, DocumentStatus::ACTUAL, { 3 });//add documents
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs[0].id, 0);
    ASSERT_EQUAL(found_docs[1].id, 2);
    ASSERT_EQUAL(found_docs[2].id, 4);
    ASSERT_EQUAL(found_docs[0].rating, 6);
    ASSERT_EQUAL(found_docs[1].rating, 8);
    ASSERT_EQUAL(found_docs[2].rating, 4);
}

void TestPredicateFilter() {
    SearchServer server;
    server.AddDocument(0, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, 4 });
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "black cat small tail loud voice"s, DocumentStatus::ACTUAL, { 14, 4, 6 });
    server.AddDocument(3, "red bird loud voice"s, DocumentStatus::ACTUAL, { 5, 16, 1 });
    server.AddDocument(4, "black big cat with brown small eyes"s, DocumentStatus::ACTUAL, { 1, 2, 9 });
    server.AddDocument(5, "black dog white tail"s, DocumentStatus::ACTUAL, { 3 });//add documents
    auto result = server.FindTopDocuments("brown dog"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0;
    status = status;
    rating = rating; });
    ASSERT_EQUAL(result.size(), 1u);
    ASSERT_EQUAL(result[0].id, 4);
}

void TestExactStatus() {
    SearchServer server;
    server.AddDocument(0, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, 4 });
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::BANNED, { 7, 2, 7 });
    server.AddDocument(2, "black cat small tail loud voice"s, DocumentStatus::IRRELEVANT, { 14, 4, 6 });
    server.AddDocument(3, "red bird loud voice"s, DocumentStatus::BANNED, { 5, 16, 1 });
    server.AddDocument(4, "black big cat with brown small eyes"s, DocumentStatus::ACTUAL, { 1, 2, 9 });
    server.AddDocument(5, "black dog white tail"s, DocumentStatus::REMOVED, { 3 });//add documents
    auto result = server.FindTopDocuments("black big dog"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(result.size(), 2u);
    ASSERT_EQUAL(result[1].id, 0);
    ASSERT_EQUAL(result[0].id, 4);
}
//1 тест осталc - right relevance
void TestRightRelevance() {
    SearchServer server;
    server.AddDocument(0, "white cat fashion collar"s, DocumentStatus::ACTUAL, { 8, 4 });
    server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 14, 4, 6 });
    //add documents
    auto result = server.FindTopDocuments("fluffy groomed cat"s);
    /*cout << result[0].id << "\t" << result[0].relevance<< endl;
    cout << result[1].id << "\t" << result[1].relevance << endl;
    cout << result[2].id << "\t" << result[2].relevance << endl;*/

    ASSERT_EQUAL(result[0].id, 1);//0.650672
    ASSERT_EQUAL(result[1].id, 2);//0.274653
    ASSERT_EQUAL(result[2].id, 0);//0.101366
    ASSERT((result[0].relevance - 0.650672) < 1e-6);
    ASSERT((result[1].relevance - 0.274653) < 1e-6);
    ASSERT((result[2].relevance - 0.101366) < 1e-6);
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestRelevanceRatingOrder);
    RUN_TEST(TestPredicateFilter);
    RUN_TEST(TestExactStatus);
    RUN_TEST(TestRightRelevance);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

