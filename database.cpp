#include "database.h"
#include <iostream>
#include <stdexcept>

Database::Database(const std::string& dbPath) : m_db(nullptr), m_dbPath(dbPath) {
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw std::runtime_error("Failed to open SQLite database at " + m_dbPath + ": " + errMsg);
    }

    // Enable WAL mode for better concurrency and foreign keys
    char* err = nullptr;
    sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err);
    sqlite3_free(err);
    sqlite3_exec(m_db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, &err);
    sqlite3_free(err);

    createTables();
    seedInitialDataIfEmpty();
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void Database::createTables() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* schema = 
        "CREATE TABLE IF NOT EXISTS disasters ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  type TEXT NOT NULL,"
        "  location TEXT NOT NULL,"
        "  severity TEXT NOT NULL,"
        "  date TEXT NOT NULL,"
        "  description TEXT,"
        "  status TEXT NOT NULL DEFAULT 'Active',"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS resources ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  category TEXT NOT NULL,"
        "  quantity INTEGER NOT NULL CHECK (quantity >= 0),"
        "  unit TEXT NOT NULL,"
        "  storage_location TEXT NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS relief_requests ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  requester_name TEXT NOT NULL,"
        "  location TEXT NOT NULL,"
        "  required_resource TEXT NOT NULL,"
        "  quantity INTEGER NOT NULL CHECK (quantity > 0),"
        "  urgency TEXT NOT NULL,"
        "  status TEXT NOT NULL DEFAULT 'Pending',"
        "  contact_info TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS distributions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  resource_id INTEGER NOT NULL,"
        "  resource_name TEXT NOT NULL,"
        "  disaster_id INTEGER NOT NULL,"
        "  disaster_name TEXT NOT NULL,"
        "  request_id INTEGER DEFAULT 0,"
        "  quantity INTEGER NOT NULL CHECK (quantity > 0),"
        "  destination TEXT NOT NULL,"
        "  date TEXT NOT NULL,"
        "  status TEXT NOT NULL,"
        "  notes TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, schema, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errStr = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Error creating database tables: " + errStr);
    }
}

bool Database::beginTransaction() {
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool Database::commitTransaction() {
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool Database::rollbackTransaction() {
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        sqlite3_free(err);
        return false;
    }
    return true;
}

// =================== DISASTERS ===================

int Database::addDisaster(const Disaster& disaster) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "INSERT INTO disasters (name, type, location, severity, date, description, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, disaster.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, disaster.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, disaster.getLocation().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, disaster.getSeverity().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, disaster.getDate().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, disaster.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, disaster.getStatus().c_str(), -1, SQLITE_TRANSIENT);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::vector<Disaster> Database::getAllDisasters() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<Disaster> list;
    const char* sql = "SELECT id, name, type, location, severity, date, description, status, created_at "
                      "FROM disasters ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string severity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        list.emplace_back(id, name, type, location, severity, date,
                          desc ? desc : "", status, createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return list;
}

std::unique_ptr<Disaster> Database::getDisasterById(int id) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "SELECT id, name, type, location, severity, date, description, status, created_at "
                      "FROM disasters WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return nullptr;

    sqlite3_bind_int(stmt, 1, id);
    std::unique_ptr<Disaster> item = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string severity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        item = std::make_unique<Disaster>(id, name, type, location, severity, date,
                                          desc ? desc : "", status, createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return item;
}

bool Database::updateDisaster(const Disaster& disaster) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "UPDATE disasters SET name=?, type=?, location=?, severity=?, "
                      "date=?, description=?, status=? WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, disaster.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, disaster.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, disaster.getLocation().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, disaster.getSeverity().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, disaster.getDate().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, disaster.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, disaster.getStatus().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, disaster.getId());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteDisaster(int id) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "DELETE FROM disasters WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// =================== RESOURCES ===================

int Database::addResource(const Resource& resource) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "INSERT INTO resources (name, category, quantity, unit, storage_location) "
                      "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, resource.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, resource.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, resource.getQuantity());
    sqlite3_bind_text(stmt, 4, resource.getUnit().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, resource.getStorageLocation().c_str(), -1, SQLITE_TRANSIENT);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::vector<Resource> Database::getAllResources() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<Resource> list;
    const char* sql = "SELECT id, name, category, quantity, unit, storage_location, created_at "
                      "FROM resources ORDER BY name ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int qty = sqlite3_column_int(stmt, 3);
        std::string unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string loc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        list.emplace_back(id, name, cat, qty, unit, loc, createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return list;
}

std::unique_ptr<Resource> Database::getResourceById(int id) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "SELECT id, name, category, quantity, unit, storage_location, created_at "
                      "FROM resources WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return nullptr;

    sqlite3_bind_int(stmt, 1, id);
    std::unique_ptr<Resource> item = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int qty = sqlite3_column_int(stmt, 3);
        std::string unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string loc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        item = std::make_unique<Resource>(id, name, cat, qty, unit, loc, createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return item;
}

bool Database::updateResource(const Resource& resource) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "UPDATE resources SET name=?, category=?, quantity=?, unit=?, storage_location=? "
                      "WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, resource.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, resource.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, resource.getQuantity());
    sqlite3_bind_text(stmt, 4, resource.getUnit().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, resource.getStorageLocation().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, resource.getId());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateResourceQuantity(int id, int newQuantity) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "UPDATE resources SET quantity = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, newQuantity);
    sqlite3_bind_int(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteResource(int id) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "DELETE FROM resources WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// =================== RELIEF REQUESTS ===================

int Database::addReliefRequest(const ReliefRequest& req) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "INSERT INTO relief_requests (requester_name, location, required_resource, quantity, urgency, status, contact_info) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, req.getRequesterName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, req.getLocation().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, req.getRequiredResource().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, req.getQuantity());
    sqlite3_bind_text(stmt, 5, req.getUrgency().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, req.getStatus().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, req.getContactInfo().c_str(), -1, SQLITE_TRANSIENT);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::vector<ReliefRequest> Database::getAllReliefRequests() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<ReliefRequest> list;
    const char* sql = "SELECT id, requester_name, location, required_resource, quantity, urgency, status, contact_info, created_at "
                      "FROM relief_requests ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string reqName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string loc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string res = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int qty = sqlite3_column_int(stmt, 4);
        std::string urgency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const char* contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        list.emplace_back(id, reqName, loc, res, qty, urgency, status,
                          contact ? contact : "", createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return list;
}

std::unique_ptr<ReliefRequest> Database::getReliefRequestById(int id) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "SELECT id, requester_name, location, required_resource, quantity, urgency, status, contact_info, created_at "
                      "FROM relief_requests WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return nullptr;

    sqlite3_bind_int(stmt, 1, id);
    std::unique_ptr<ReliefRequest> item = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string reqName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string loc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string res = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int qty = sqlite3_column_int(stmt, 4);
        std::string urgency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const char* contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        item = std::make_unique<ReliefRequest>(id, reqName, loc, res, qty, urgency, status,
                                               contact ? contact : "", createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return item;
}

bool Database::updateReliefRequestStatus(int id, const std::string& status) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "UPDATE relief_requests SET status = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteReliefRequest(int id) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "DELETE FROM relief_requests WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// =================== DISTRIBUTIONS ===================

int Database::addDistribution(const Distribution& dist) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    const char* sql = "INSERT INTO distributions (resource_id, resource_name, disaster_id, disaster_name, "
                      "request_id, quantity, destination, date, status, notes) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, dist.getResourceId());
    sqlite3_bind_text(stmt, 2, dist.getResourceName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, dist.getDisasterId());
    sqlite3_bind_text(stmt, 4, dist.getDisasterName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, dist.getRequestId());
    sqlite3_bind_int(stmt, 6, dist.getQuantity());
    sqlite3_bind_text(stmt, 7, dist.getDestination().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, dist.getDate().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, dist.getStatus().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, dist.getNotes().c_str(), -1, SQLITE_TRANSIENT);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return id;
}

std::vector<Distribution> Database::getAllDistributions() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<Distribution> list;
    const char* sql = "SELECT id, resource_id, resource_name, disaster_id, disaster_name, "
                      "request_id, quantity, destination, date, status, notes, created_at "
                      "FROM distributions ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int resId = sqlite3_column_int(stmt, 1);
        std::string resName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int disId = sqlite3_column_int(stmt, 3);
        std::string disName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        int reqId = sqlite3_column_int(stmt, 5);
        int qty = sqlite3_column_int(stmt, 6);
        std::string dest = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        std::string dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        std::string st = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        const char* notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));

        list.emplace_back(id, resId, resName, disId, disName, reqId, qty,
                          dest, dt, st, notes ? notes : "", createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Distribution> Database::getDistributionsByDisaster(int disasterId) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<Distribution> list;
    const char* sql = "SELECT id, resource_id, resource_name, disaster_id, disaster_name, "
                      "request_id, quantity, destination, date, status, notes, created_at "
                      "FROM distributions WHERE disaster_id = ? ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, disasterId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int resId = sqlite3_column_int(stmt, 1);
        std::string resName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int disId = sqlite3_column_int(stmt, 3);
        std::string disName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        int reqId = sqlite3_column_int(stmt, 5);
        int qty = sqlite3_column_int(stmt, 6);
        std::string dest = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        std::string dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        std::string st = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        const char* notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));

        list.emplace_back(id, resId, resName, disId, disName, reqId, qty,
                          dest, dt, st, notes ? notes : "", createdAt ? createdAt : "");
    }
    sqlite3_finalize(stmt);
    return list;
}

// =================== DATA SEEDING ===================

void Database::seedInitialDataIfEmpty() {
    // Check if disasters table is empty
    int disasterCount = 0;
    const char* countSql = "SELECT COUNT(*) FROM disasters;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, countSql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            disasterCount = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (disasterCount == 0) {
        resetToSampleData();
    }
}

void Database::clearAllData() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    char* err = nullptr;
    sqlite3_exec(m_db, "DELETE FROM distributions;", nullptr, nullptr, &err);
    sqlite3_free(err);
    sqlite3_exec(m_db, "DELETE FROM relief_requests;", nullptr, nullptr, &err);
    sqlite3_free(err);
    sqlite3_exec(m_db, "DELETE FROM resources;", nullptr, nullptr, &err);
    sqlite3_free(err);
    sqlite3_exec(m_db, "DELETE FROM disasters;", nullptr, nullptr, &err);
    sqlite3_free(err);
}

void Database::resetToSampleData() {
    clearAllData();

    // 1. Seed Sample Disasters
    addDisaster(Disaster(0, "Northern Flash Floods", "Flood", "River Basin District", "Critical", "2026-10-18", "Rapid river overflow submerging residential settlements and blocking highways.", "Active", ""));
    addDisaster(Disaster(0, "Coastal Cyclone Storm Surge", "Cyclone", "Eastern Seaboard", "High", "2026-10-15", "Category 4 tropical storm with 140 km/h winds and coastal flooding.", "Active", ""));
    addDisaster(Disaster(0, "Hillside Landslide Incident", "Landslide", "Mountview Foothills", "Medium", "2026-10-12", "Debris flow cutting off power lines and access to three rural communities.", "Contained", ""));
    addDisaster(Disaster(0, "Industrial Sector Wildfire", "Wildfire", "Pine Ridge Valley", "High", "2026-10-09", "Dry weather fire advancing near storage depots.", "Resolved", ""));

    // 2. Seed Sample Resources (Food, Water, Medicine, Shelter, Equipment)
    addResource(Resource(0, "Emergency Food Packets", "Food", 1250, "packets", "Central Warehouse A", ""));
    addResource(Resource(0, "Purified Drinking Water", "Water", 2800, "liters", "Water Depot North", ""));
    addResource(Resource(0, "First Aid & Trauma Kits", "Medical", 340, "kits", "Medical Supply Unit", ""));
    addResource(Resource(0, "Thermal Emergency Blankets", "Shelter", 850, "blankets", "Central Warehouse A", ""));
    addResource(Resource(0, "Inflatable Rescue Boats", "Rescue Equipment", 18, "units", "Marine Response Bay", ""));
    addResource(Resource(0, "Portable Power Generators", "Equipment", 24, "units", "Logistics Base 2", ""));
    addResource(Resource(0, "Water Purification Tablets", "Medical", 4500, "tablets", "Medical Supply Unit", ""));

    // 3. Seed Sample Relief Requests
    addReliefRequest(ReliefRequest(0, "Community Shelter #4", "River Basin District", "Emergency Food Packets", 300, "Urgent", "Pending", "+91 98765 43210", ""));
    addReliefRequest(ReliefRequest(0, "Dr. Sarah (Field Clinic)", "Eastern Seaboard", "First Aid & Trauma Kits", 60, "High", "In Progress", "+91 91234 56789", ""));
    addReliefRequest(ReliefRequest(0, "Highland Evacuation Center", "Mountview Foothills", "Thermal Emergency Blankets", 150, "Medium", "Approved", "+91 99887 76655", ""));
    addReliefRequest(ReliefRequest(0, "Riverside Volunteer Camp", "River Basin District", "Purified Drinking Water", 500, "Urgent", "Pending", "+91 97766 55443", ""));

    // 4. Seed Initial Distributions
    addDistribution(Distribution(0, 1, "Emergency Food Packets", 1, "Northern Flash Floods", 0, 200, "River Basin Camp Alpha", "2026-10-19", "Delivered", "Initial rapid response ration drop.", ""));
    addDistribution(Distribution(0, 2, "Purified Drinking Water", 1, "Northern Flash Floods", 0, 600, "River Basin Camp Alpha", "2026-10-19", "Delivered", "Tanker and bottled water delivery.", ""));
    addDistribution(Distribution(0, 3, "First Aid & Trauma Kits", 2, "Coastal Cyclone Storm Surge", 2, 40, "Eastern Seaboard Medical Post", "2026-10-20", "In Transit", "Dispatched via medical convoy.", ""));
}
