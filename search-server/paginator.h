#pragma once
#include <vector>
#include <iostream>

template <typename It>
class IteratorRange {
public:
    IteratorRange (It begin_, It end_)
        : begin(begin_), end(end_)
        {    
    }
    It Begin() {
        return begin;
    }
    It End () {
        return end;
    }
    int Size () {
        return std::distance(begin, end);
    }
private:
    It begin;
    It end;
    };

template <typename It>
std::ostream& operator<<(std::ostream& output, IteratorRange <It> point) {//оператор вывода
    for (auto i = point.Begin(); i != point.End(); ++i) {
        output << *i;    
    }
    return output;
}

template <typename It>
class Paginator {
public:
    Paginator (It begin, It end, size_t page_size) {//конструктор
        for (It page_begin = begin; page_begin < end; page_begin+=page_size) {
            //если страница заканчивается, надо ее заполнить, а если первоначальный диапазон закончен, надо остатки разместить на странице
            It page_end = std::next(page_begin, std::min(page_size, static_cast<size_t>(std::distance(page_begin, end))));
            IteratorRange page(page_begin, page_end);
            result.push_back(page);
        }
        //надо помещать в вектор диапазоны, пока счетчик не станет равен end. Диапазон - это IteratorRange, а счетчик - это It
    }
    auto begin() const{
        return result.begin();
    }
    auto end() const{
        return result.end();
    }
    int Size() const{
        return result.size();
    }
private:
    std::vector <IteratorRange<It>> result;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}

