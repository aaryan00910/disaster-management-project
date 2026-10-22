#pragma once

#include <string>
#include <crow_all.h>

/**
 * @brief Abstract Base Class representing an entity in the Disaster Relief System.
 * Demonstrates OOP Inheritance and Polymorphism.
 */
class BaseEntity {
protected:
    int m_id;
    std::string m_createdAt;

public:
    BaseEntity() : m_id(0), m_createdAt("") {}
    BaseEntity(int id, const std::string& createdAt) : m_id(id), m_createdAt(createdAt) {}
    virtual ~BaseEntity() = default;

    // Getters and Setters (Encapsulation)
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    std::string getCreatedAt() const { return m_createdAt; }
    void setCreatedAt(const std::string& createdAt) { m_createdAt = createdAt; }

    // Pure virtual method for polymorphic JSON serialization
    virtual crow::json::wvalue toCrowJson() const = 0;
};
