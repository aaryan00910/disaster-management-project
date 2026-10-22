#pragma once

#include "base_entity.h"
#include <string>

/**
 * @brief Represents a request for disaster relief aid.
 * Inherits from BaseEntity, with lifecycle status management.
 */
class ReliefRequest : public BaseEntity {
private:
    std::string m_requesterName;
    std::string m_location;
    std::string m_requiredResource;
    int m_quantity;
    std::string m_urgency;      // Low, Medium, High, Urgent
    std::string m_status;       // Pending, Approved, In Progress, Completed, Rejected
    std::string m_contactInfo;

public:
    ReliefRequest();
    ReliefRequest(int id, const std::string& requesterName, const std::string& location,
                  const std::string& requiredResource, int quantity,
                  const std::string& urgency, const std::string& status,
                  const std::string& contactInfo, const std::string& createdAt);

    // Getters
    std::string getRequesterName() const { return m_requesterName; }
    std::string getLocation() const { return m_location; }
    std::string getRequiredResource() const { return m_requiredResource; }
    int getQuantity() const { return m_quantity; }
    std::string getUrgency() const { return m_urgency; }
    std::string getStatus() const { return m_status; }
    std::string getContactInfo() const { return m_contactInfo; }

    // Setters
    void setRequesterName(const std::string& name) { m_requesterName = name; }
    void setLocation(const std::string& location) { m_location = location; }
    void setRequiredResource(const std::string& resource) { m_requiredResource = resource; }
    void setQuantity(int quantity);
    void setUrgency(const std::string& urgency);
    void setStatus(const std::string& status);
    void setContactInfo(const std::string& contact) { m_contactInfo = contact; }

    // Polymorphic serialization
    crow::json::wvalue toCrowJson() const override;

    static bool isValidStatus(const std::string& status);
    static bool isValidUrgency(const std::string& urgency);
};
