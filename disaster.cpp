#include "disaster.h"
#include <stdexcept>
#include <algorithm>

Disaster::Disaster()
    : BaseEntity(), m_name(""), m_type("General"), m_location(""),
      m_severity("Medium"), m_date(""), m_description(""), m_status("Active") {}

Disaster::Disaster(int id, const std::string& name, const std::string& type,
                   const std::string& location, const std::string& severity,
                   const std::string& date, const std::string& description,
                   const std::string& status, const std::string& createdAt)
    : BaseEntity(id, createdAt), m_name(name), m_type(type),
      m_location(location), m_date(date), m_description(description), m_status(status) {
    setSeverity(severity);
}

void Disaster::setSeverity(const std::string& severity) {
    if (!isValidSeverity(severity)) {
        m_severity = "Medium";
    } else {
        m_severity = severity;
    }
}

bool Disaster::isValidSeverity(const std::string& severity) {
    return (severity == "Low" || severity == "Medium" || 
            severity == "High" || severity == "Critical");
}

crow::json::wvalue Disaster::toCrowJson() const {
    crow::json::wvalue json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["type"] = m_type;
    json["location"] = m_location;
    json["severity"] = m_severity;
    json["date"] = m_date;
    json["description"] = m_description;
    json["status"] = m_status;
    json["createdAt"] = m_createdAt;
    return json;
}
