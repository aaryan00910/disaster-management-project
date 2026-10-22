#pragma once

#include "database.h"
#include <memory>
#include <vector>
#include <string>
#include <crow_all.h>

/**
 * @brief DisasterReliefSystem coordinates business logic, transactions, 
 * stock validations, and analytical reporting.
 * Demonstrates Service Layer / Facade OOP pattern.
 */
class DisasterReliefSystem {
private:
    std::shared_ptr<Database> m_db;

public:
    explicit DisasterReliefSystem(std::shared_ptr<Database> db);

    // Business Logic: Allocation
    struct AllocationResult {
        bool success;
        std::string message;
        int distributionId;
        int remainingStock;
    };

    AllocationResult allocateResource(int resourceId, int disasterId, int quantity,
                                     const std::string& destination, int requestId = 0,
                                     const std::string& notes = "");

    // Dashboard & Metrics
    crow::json::wvalue getDashboardSummary();
    crow::json::wvalue getDetailedReport();

    // Pass-through & Domain Queries
    std::shared_ptr<Database> getDatabase() { return m_db; }
};
