#include "relief_request.h"
#include <stdexcept>

ReliefRequest::ReliefRequest()
    : BaseEntity(), m_requesterName(""), m_location(""), m_requiredResource(""),
      m_quantity(1), m_urgency("Medium"), m_status("Pending"), m_contactInfo("") {}

ReliefRequest::ReliefRequest(int id, const std::string& requesterName, const std::string& location,
                             const std::string& requiredResource, int quantity,
                             const std::string& urgency, const std::string& status,
                             const std::string& contactInfo, const std::string& createdAt)
    : BaseEntity(id, createdAt), m_requesterName(requesterName), m_location(location),
      m_requiredResource(requiredResource), m_contactInfo(contactInfo) {
    setQuantity(quantity);
    setUrgency(urgency);
    setStatus(status);
}

void ReliefRequest::setQuantity(int quantity) {
    if (quantity <= 0) {
        throw std::invalid_argument("Request quantity must be greater than zero.");
    }
    m_quantity = quantity;
}

void ReliefRequest::setUrgency(const std::string& urgency) {
    if (isValidUrgency(urgency)) {
        m_urgency = urgency;
    } else {
        m_urgency = "Medium";
    }
}

void ReliefRequest::setStatus(const std::string& status) {
    if (isValidStatus(status)) {
        m_status = status;
    } else {
        m_status = "Pending";
    }
}

bool ReliefRequest::isValidStatus(const std::string& status) {
    return (status == "Pending" || status == "Approved" ||
            status == "In Progress" || status == "Completed" ||
            status == "Rejected");
}

bool ReliefRequest::isValidUrgency(const std::string& urgency) {
    return (urgency == "Low" || urgency == "Medium" ||
            urgency == "High" || urgency == "Urgent");
}

crow::json::wvalue ReliefRequest::toCrowJson() const {
    crow::json::wvalue json;
    json["id"] = m_id;
    json["requesterName"] = m_requesterName;
    json["location"] = m_location;
    json["requiredResource"] = m_requiredResource;
    json["quantity"] = m_quantity;
    json["urgency"] = m_urgency;
    json["status"] = m_status;
    json["contactInfo"] = m_contactInfo;
    json["createdAt"] = m_createdAt;
    return json;
}
