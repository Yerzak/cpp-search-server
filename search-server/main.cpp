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
#include <cstdlib>
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
void TestRightRating() {
    SearchServer server;
    server.AddDocument(0, "white cat white tail"s, DocumentStatus::ACTUAL, { 8, 10 });//rating = 9
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 1, 7 });//rating = 5
    const auto found_docs = server.FindTopDocuments("white"s);
    const auto doc0 = found_docs[0];
    const int result0 = (8 + 10) / 2;
    ASSERT_EQUAL(doc0.rating, result0);
    const auto doc1 = found_docs[1];
    const auto result1 = (7 + 1 + 7) / 3;
    ASSERT_EQUAL(doc1.rating, result1);
}
void TestAddDocument() {
    SearchServer server;
    int result = server.GetDocumentCount();
    ASSERT(result == 0);
    server.AddDocument(0, "white cat white tail"s, DocumentStatus::ACTUAL, { 8, 10 });//add first doc
    result = server.GetDocumentCount();
    ASSERT(result == 1);
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 1, 7 });//add second doc
    //ASSERT(server.GetDocumentCount()==2);
    server.AddDocument(4, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, 4 });
    server.AddDocument(6, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(12, "black cat small tail loud voice"s, DocumentStatus::ACTUAL, { 14, 4, 6 });
    server.AddDocument(7, "red bird loud voice"s, DocumentStatus::ACTUAL, { 5, 16, 1 });
    server.AddDocument(3, "black big cat with brown small eyes"s, DocumentStatus::ACTUAL, { 1, 2, 9 }); // + 5 docs
    result = server.GetDocumentCount();
    ASSERT(result == 7);
}
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
void TestRelevanceOrder() {
    SearchServer server;
    server.AddDocument(0, "white cat big eyes"s, DocumentStatus::ACTUAL, { 8, 4 });//the most relevance doc for searching "cat"
    server.AddDocument(1, "white dog big ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "black cat small tail loud voice"s, DocumentStatus::ACTUAL, { 14, 4, 6 });//second doc for relevance for searching "cat"
    server.AddDocument(3, "red bird loud voice"s, DocumentStatus::ACTUAL, { 5, 16, 1 });
    server.AddDocument(4, "black big cat with brown small eyes"s, DocumentStatus::ACTUAL, { 1, 2, 9 });//the last for searching "cat"
    server.AddDocument(5, "black dog white tail"s, DocumentStatus::ACTUAL, { 3 });//add documents
    const auto found_docs = server.FindTopDocuments("cat"s);
    vector <int> results = { 0, 2, 4 }; //result docs order: 0, 2, 4
    int counter = 0;
    for (auto i : found_docs) {
        ASSERT_EQUAL(i.id, results[counter]);
        ++counter;
    }
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
    //порядок документов в результате поиска согласно релевантности: 1, 2, 0
    ASSERT_EQUAL(result[0].id, 1);
    ASSERT_EQUAL(result[1].id, 2);
    ASSERT_EQUAL(result[2].id, 0);
    //нужно добавить константу, принимающую результат формулы

    double all_docs = 3;//всего документов
    double idf_for_fluffy = log(all_docs / 1);
    double idf_for_groomed = log(all_docs / 1);
    double idf_for_cat = log(all_docs / 2);//считаем IDF каждого слова запроса


    double tf0_for_fluffy = 0.0 / 4.0;
    double tf0_for_groomed = 0.0 / 4.0;
    double tf0_for_cat = 1.0 / 4.0;
    /*считаем TF каждого слова в первом добавленном документе (он же последний в результате)*/
    double relevance_of_last_result = (idf_for_fluffy * tf0_for_fluffy) + (idf_for_groomed * tf0_for_groomed) + (idf_for_cat * tf0_for_cat);
    //должно быть 0.101366
    double real_difference_of_last = abs(result[2].relevance - relevance_of_last_result);//вычисляем разность по модулю

    double tf1_for_fluffy = 2.0 / 4.0;
    double tf1_for_groomed = 0.0 / 4.0;
    double tf1_for_cat = 1.0 / 4.0;
    /*считаем TF каждого слова во втором добавленном документе (он же первый в результате)*/
    double relevance_of_first_result = (idf_for_fluffy * tf1_for_fluffy) + (idf_for_groomed * tf1_for_groomed) + (idf_for_cat * tf1_for_cat);
    //должно быть 0.650672
    double real_difference_of_first = abs(result[0].relevance - relevance_of_first_result);//вычисляем разность по модулю    

    double tf2_for_fluffy = 0.0 / 4.0;
    double tf2_for_groomed = 1.0 / 4.0;
    double tf2_for_cat = 0.0 / 4.0;
    /*считаем TF каждого слова в третьем добавленном документе (он же второй в результате)*/
    double relevance_of_mid_result = (idf_for_fluffy * tf2_for_fluffy) + (idf_for_groomed * tf2_for_groomed) + (idf_for_cat * tf2_for_cat);
    //должно быть 0.274653
    double real_difference_of_mid = abs(result[1].relevance - relevance_of_mid_result);//вычисляем разность по модулю

    double minimal_difference = 1e-6;
    ASSERT(real_difference_of_first < minimal_difference);
    ASSERT(real_difference_of_mid < minimal_difference);
    ASSERT(real_difference_of_last < minimal_difference);
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestRightRating);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestRelevanceOrder);
    RUN_TEST(TestPredicateFilter);
    RUN_TEST(TestExactStatus);
    RUN_TEST(TestRightRelevance);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

