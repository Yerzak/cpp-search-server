//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎
#include "document.h"


Document::Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

std::ostream& operator<<(std::ostream& output, const Document& point) {// оператор вывода
    output << "{ document_id = "s << point.id << ", relevance = "s << point.relevance << ", rating = "s << point.rating << " }"s;    
    return output;
}
