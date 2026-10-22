#include "distribution.h"
#include <stdexcept>

Distribution::Distribution()
    : BaseEntity(), m_resourceId(0), m_resourceName(""), m_disasterId(0),
      m_disasterName(""), m_requestId(0), m_quantity(0), m_destination(""),
      m_date(""), m_status("Dispatched"), m_notes("") {}

Distribution::Distribution(int id, int resourceId, const std::string& resourceName,
                           int disasterId, const std::string& disasterName,
                           int requestId, int quantity, const std::string& destination,
                           const std::string& date, const std::string& status,
                           const std::string& notes, const std::string& createdAt)
    : BaseEntity(id, createdAt), m_resourceId(resourceId), m_resourceName(resourceName),
      m_disasterId(disasterId), m_disasterName(disasterName), m_requestId(requestId),
      m_destination(destination), m_date(date), m_status(status), m_notes(notes) {
    setQuantity(quantity);
}

void Distribution::setQuantity(int quantity) {
    if (quantity <= 0) {
        throw std::invalid_argument("Distribution quantity must be positive.");
    }
    m_quantity = quantity;
}

crow::json::wvalue Distribution::toCrowJson() const {
    crow::json::wvalue json;
    json["id"] = m_id;
    json["resourceId"] = m_resourceId;
    json["resourceName"] = m_resourceName;
    json["disasterId"] = m_disasterId;
    json["disasterName"] = m_disasterName;
    json["requestId"] = m_requestId;
    json["quantity"] = m_quantity;
    json["destination"] = m_destination;
    json["date"] = m_date;
    json["status"] = m_status;
    json["notes"] = m_notes;
    json["createdAt"] = m_createdAt;
    return json;
}
