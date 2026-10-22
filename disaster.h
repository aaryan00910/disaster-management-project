#pragma once

#include "base_entity.h"
#include <string>

/**
 * @brief Represents a disaster event.
 * Inherits from BaseEntity, demonstrating Single Inheritance and Polymorphism.
 */
class Disaster : public BaseEntity {
private:
    std::string m_name;
    std::string m_type;        // e.g., Flood, Earthquake, Cyclone, Wildfire, Landslide
    std::string m_location;
    std::string m_severity;    // Low, Medium, High, Critical
    std::string m_date;        // YYYY-MM-DD
    std::string m_description;
    std::string m_status;      // Active, Contained, Resolved

public:
    Disaster();
    Disaster(int id, const std::string& name, const std::string& type,
             const std::string& location, const std::string& severity,
             const std::string& date, const std::string& description,
             const std::string& status, const std::string& createdAt);

    // Getters
    std::string getName() const { return m_name; }
    std::string getType() const { return m_type; }
    std::string getLocation() const { return m_location; }
    std::string getSeverity() const { return m_severity; }
    std::string getDate() const { return m_date; }
    std::string getDescription() const { return m_description; }
    std::string getStatus() const { return m_status; }

    // Setters with validation
    void setName(const std::string& name) { m_name = name; }
    void setType(const std::string& type) { m_type = type; }
    void setLocation(const std::string& location) { m_location = location; }
    void setSeverity(const std::string& severity);
    void setDate(const std::string& date) { m_date = date; }
    void setDescription(const std::string& description) { m_description = description; }
    void setStatus(const std::string& status) { m_status = status; }

    // Polymorphic implementation
    crow::json::wvalue toCrowJson() const override;

    // Helper validation
    static bool isValidSeverity(const std::string& severity);
};
