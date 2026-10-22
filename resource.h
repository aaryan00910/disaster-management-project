#pragma once

#include "base_entity.h"
#include <string>
#include <stdexcept>

/**
 * @brief Represents an emergency relief inventory item.
 * Demonstrates Encapsulation and business rule enforcement (non-negative stock).
 */
class Resource : public BaseEntity {
private:
    std::string m_name;
    std::string m_category;        // Food, Water, Medicine, Shelter, Equipment, Clothing
    int m_quantity;               // Must be non-negative
    std::string m_unit;            // packets, liters, kits, blankets, units
    std::string m_storageLocation; // Warehouse A, Central Depot, etc.

public:
    Resource();
    Resource(int id, const std::string& name, const std::string& category,
             int quantity, const std::string& unit,
             const std::string& storageLocation, const std::string& createdAt);

    // Getters
    std::string getName() const { return m_name; }
    std::string getCategory() const { return m_category; }
    int getQuantity() const { return m_quantity; }
    std::string getUnit() const { return m_unit; }
    std::string getStorageLocation() const { return m_storageLocation; }

    // Setters with validation
    void setName(const std::string& name) { m_name = name; }
    void setCategory(const std::string& category) { m_category = category; }
    void setQuantity(int quantity);
    void setUnit(const std::string& unit) { m_unit = unit; }
    void setStorageLocation(const std::string& location) { m_storageLocation = location; }

    // Domain operations
    bool hasSufficientStock(int requiredQty) const;
    void addStock(int qty);
    void deductStock(int qty);

    // Polymorphic serialization
    crow::json::wvalue toCrowJson() const override;
};
