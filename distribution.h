#pragma once

#include "base_entity.h"
#include <string>

/**
 * @brief Represents an allocation of resources to a disaster site or relief request.
 * Inherits from BaseEntity, preserving an audit trail of relief aid distribution.
 */
class Distribution : public BaseEntity {
private:
    int m_resourceId;
    std::string m_resourceName;
    int m_disasterId;
    std::string m_disasterName;
    int m_requestId;            // 0 if direct disaster allocation
    int m_quantity;
    std::string m_destination;
    std::string m_date;         // YYYY-MM-DD
    std::string m_status;       // Dispatched, Delivered, In Transit
    std::string m_notes;

public:
    Distribution();
    Distribution(int id, int resourceId, const std::string& resourceName,
                 int disasterId, const std::string& disasterName,
                 int requestId, int quantity, const std::string& destination,
                 const std::string& date, const std::string& status,
                 const std::string& notes, const std::string& createdAt);

    // Getters
    int getResourceId() const { return m_resourceId; }
    std::string getResourceName() const { return m_resourceName; }
    int getDisasterId() const { return m_disasterId; }
    std::string getDisasterName() const { return m_disasterName; }
    int getRequestId() const { return m_requestId; }
    int getQuantity() const { return m_quantity; }
    std::string getDestination() const { return m_destination; }
    std::string getDate() const { return m_date; }
    std::string getStatus() const { return m_status; }
    std::string getNotes() const { return m_notes; }

    // Setters
    void setResourceId(int resId) { m_resourceId = resId; }
    void setResourceName(const std::string& name) { m_resourceName = name; }
    void setDisasterId(int disId) { m_disasterId = disId; }
    void setDisasterName(const std::string& name) { m_disasterName = name; }
    void setRequestId(int reqId) { m_requestId = reqId; }
    void setQuantity(int quantity);
    void setDestination(const std::string& dest) { m_destination = dest; }
    void setDate(const std::string& date) { m_date = date; }
    void setStatus(const std::string& status) { m_status = status; }
    void setNotes(const std::string& notes) { m_notes = notes; }

    // Polymorphic serialization
    crow::json::wvalue toCrowJson() const override;
};
