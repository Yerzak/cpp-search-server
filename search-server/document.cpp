#include "document.h"

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<<(std::ostream& output, const Document& point) {// оператор вывода
    output << std::string("{ document_id = ") << point.id << std::string(", relevance = ") << point.relevance << std::string(", rating = ") << point.rating << std::string(" }");
    return output;
}
