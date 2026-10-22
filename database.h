#pragma once

#include "disaster.h"
#include "resource.h"
#include "relief_request.h"
#include "distribution.h"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

/**
 * @brief Database class that encapsulates SQLite3 connection, parameterized queries,
 * and CRUD operations for all entities.
 * Demonstrates the RAII idiom and Separation of Concerns.
 */
class Database {
private:
    sqlite3* m_db;
    std::string m_dbPath;
    std::mutex m_dbMutex; // Thread-safety for multi-threaded Crow web server

    void createTables();
    void seedInitialDataIfEmpty();

public:
    explicit Database(const std::string& dbPath);
    ~Database();

    // Prevent copy (encapsulation of unique database connection handle)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Transaction management
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Disaster CRUD operations (using Parameterized SQL)
    int addDisaster(const Disaster& disaster);
    std::vector<Disaster> getAllDisasters();
    std::unique_ptr<Disaster> getDisasterById(int id);
    bool updateDisaster(const Disaster& disaster);
    bool deleteDisaster(int id);

    // Resource Inventory CRUD operations
    int addResource(const Resource& resource);
    std::vector<Resource> getAllResources();
    std::unique_ptr<Resource> getResourceById(int id);
    bool updateResource(const Resource& resource);
    bool updateResourceQuantity(int id, int newQuantity);
    bool deleteResource(int id);

    // Relief Request Operations
    int addReliefRequest(const ReliefRequest& request);
    std::vector<ReliefRequest> getAllReliefRequests();
    std::unique_ptr<ReliefRequest> getReliefRequestById(int id);
    bool updateReliefRequestStatus(int id, const std::string& status);
    bool deleteReliefRequest(int id);

    // Resource Distribution Operations
    int addDistribution(const Distribution& distribution);
    std::vector<Distribution> getAllDistributions();
    std::vector<Distribution> getDistributionsByDisaster(int disasterId);

    // Seed and Data Management
    void resetToSampleData();
    void clearAllData();
};
