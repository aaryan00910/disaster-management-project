#include "resource.h"

Resource::Resource()
    : BaseEntity(), m_name(""), m_category("General"), m_quantity(0),
      m_unit("units"), m_storageLocation("") {}

Resource::Resource(int id, const std::string& name, const std::string& category,
                   int quantity, const std::string& unit,
                   const std::string& storageLocation, const std::string& createdAt)
    : BaseEntity(id, createdAt), m_name(name), m_category(category),
      m_unit(unit), m_storageLocation(storageLocation) {
    setQuantity(quantity);
}

void Resource::setQuantity(int quantity) {
    if (quantity < 0) {
        throw std::invalid_argument("Resource quantity cannot be negative.");
    }
    m_quantity = quantity;
}

bool Resource::hasSufficientStock(int requiredQty) const {
    return (requiredQty > 0 && m_quantity >= requiredQty);
}

void Resource::addStock(int qty) {
    if (qty < 0) {
        throw std::invalid_argument("Stock addition amount cannot be negative.");
    }
    m_quantity += qty;
}

void Resource::deductStock(int qty) {
    if (qty < 0) {
        throw std::invalid_argument("Stock deduction amount cannot be negative.");
    }
    if (qty > m_quantity) {
        throw std::runtime_error("Insufficient inventory: requested quantity exceeds available stock.");
    }
    m_quantity -= qty;
}

crow::json::wvalue Resource::toCrowJson() const {
    crow::json::wvalue json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["category"] = m_category;
    json["quantity"] = m_quantity;
    json["unit"] = m_unit;
    json["storageLocation"] = m_storageLocation;
    json["createdAt"] = m_createdAt;
    return json;
}
