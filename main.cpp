#include "crow_all.h"
#include "database.h"
#include "system.h"
#include "disaster.h"
#include "resource.h"
#include "relief_request.h"
#include "distribution.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

// Helper to read file contents
std::string readFileContents(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Helper to attach CORS headers
void setCorsHeaders(crow::response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

// JSON value extraction helpers
std::string getJsonString(const crow::json::rvalue& body, const std::string& key, const std::string& defaultVal = "") {
    if (body.has(key)) {
        return std::string(body[key].s());
    }
    return defaultVal;
}

int getJsonInt(const crow::json::rvalue& body, const std::string& key, int defaultVal = 0) {
    if (body.has(key)) {
        return static_cast<int>(body[key].i());
    }
    return defaultVal;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    // Ensure data directory exists
    if (!fs::exists("data")) {
        fs::create_directories("data");
    }

    std::cout << "======================================================" << std::endl;
    std::cout << "  Disaster Relief and Resource Management System" << std::endl;
    std::cout << "  Backend: C++ / Crow Web Framework" << std::endl;
    std::cout << "  Database: SQLite (data/disaster_relief.db)" << std::endl;
    std::cout << "======================================================" << std::endl;

    auto db = std::make_shared<Database>("data/disaster_relief.db");
    DisasterReliefSystem reliefSystem(db);

    crow::SimpleApp app;

    // ----------------------------------------------------
    // Frontend Static Assets
    // ----------------------------------------------------
    CROW_ROUTE(app, "/")
    ([](const crow::request& req, crow::response& res) {
        std::string html = readFileContents("frontend/index.html");
        if (html.empty()) {
            res.code = 404;
            res.write("frontend/index.html not found.");
        } else {
            res.code = 200;
            res.set_header("Content-Type", "text/html; charset=utf-8");
            res.write(html);
        }
        res.end();
    });

    CROW_ROUTE(app, "/style.css")
    ([](const crow::request& req, crow::response& res) {
        std::string css = readFileContents("frontend/style.css");
        res.set_header("Content-Type", "text/css; charset=utf-8");
        res.write(css);
        res.end();
    });

    CROW_ROUTE(app, "/script.js")
    ([](const crow::request& req, crow::response& res) {
        std::string js = readFileContents("frontend/script.js");
        res.set_header("Content-Type", "application/javascript; charset=utf-8");
        res.write(js);
        res.end();
    });

    // Handle OPTIONS for CORS
    CROW_ROUTE(app, "/api/<path>")
    .methods("OPTIONS"_method)
    ([](const crow::request& req, crow::response& res, std::string) {
        setCorsHeaders(res);
        res.code = 204;
        res.end();
    });

    // ----------------------------------------------------
    // Dashboard Stats & Reports API
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/stats")
    .methods("GET"_method)
    ([&reliefSystem](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto summary = reliefSystem.getDashboardSummary();
        res.set_header("Content-Type", "application/json");
        res.write(summary.dump());
        res.end();
    });

    CROW_ROUTE(app, "/api/reports")
    .methods("GET"_method)
    ([&reliefSystem](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto report = reliefSystem.getDetailedReport();
        res.set_header("Content-Type", "application/json");
        res.write(report.dump());
        res.end();
    });

    CROW_ROUTE(app, "/api/reset-sample-data")
    .methods("POST"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        db->resetToSampleData();
        crow::json::wvalue resp;
        resp["success"] = true;
        resp["message"] = "Sample data has been re-seeded successfully.";
        res.set_header("Content-Type", "application/json");
        res.write(resp.dump());
        res.end();
    });

    // ----------------------------------------------------
    // Disasters REST API
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/disasters")
    .methods("GET"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto disasters = db->getAllDisasters();
        std::vector<crow::json::wvalue> list;
        for (const auto& d : disasters) {
            list.push_back(d.toCrowJson());
        }
        crow::json::wvalue result;
        result["disasters"] = std::move(list);
        res.set_header("Content-Type", "application/json");
        res.write(result.dump());
        res.end();
    });

    CROW_ROUTE(app, "/api/disasters/<int>")
    .methods("GET"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        auto item = db->getDisasterById(id);
        if (!item) {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Disaster not found.";
            res.write(err.dump());
        } else {
            res.set_header("Content-Type", "application/json");
            res.write(item->toCrowJson().dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/disasters")
    .methods("POST"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid JSON payload.";
            res.write(err.dump());
            res.end();
            return;
        }

        std::string name = getJsonString(body, "name");
        std::string type = getJsonString(body, "type", "General");
        std::string location = getJsonString(body, "location");
        std::string severity = getJsonString(body, "severity", "Medium");
        std::string date = getJsonString(body, "date");
        std::string desc = getJsonString(body, "description");
        std::string status = getJsonString(body, "status", "Active");

        if (name.empty() || location.empty()) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Disaster name and location are required.";
            res.write(err.dump());
            res.end();
            return;
        }

        Disaster d(0, name, type, location, severity, date, desc, status, "");
        int id = db->addDisaster(d);
        if (id <= 0) {
            res.code = 500;
            crow::json::wvalue err;
            err["error"] = "Database insert failed.";
            res.write(err.dump());
        } else {
            res.code = 201;
            d.setId(id);
            res.set_header("Content-Type", "application/json");
            res.write(d.toCrowJson().dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/disasters/<int>")
    .methods("PUT"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        auto existing = db->getDisasterById(id);
        if (!existing) {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Disaster not found.";
            res.write(err.dump());
            res.end();
            return;
        }

        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid JSON payload.";
            res.write(err.dump());
            res.end();
            return;
        }

        std::string name = getJsonString(body, "name", existing->getName());
        std::string type = getJsonString(body, "type", existing->getType());
        std::string location = getJsonString(body, "location", existing->getLocation());
        std::string severity = getJsonString(body, "severity", existing->getSeverity());
        std::string date = getJsonString(body, "date", existing->getDate());
        std::string desc = getJsonString(body, "description", existing->getDescription());
        std::string status = getJsonString(body, "status", existing->getStatus());

        Disaster updated(id, name, type, location, severity, date, desc, status, existing->getCreatedAt());
        if (db->updateDisaster(updated)) {
            res.set_header("Content-Type", "application/json");
            res.write(updated.toCrowJson().dump());
        } else {
            res.code = 500;
            crow::json::wvalue err;
            err["error"] = "Failed to update disaster.";
            res.write(err.dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/disasters/<int>")
    .methods("DELETE"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        if (db->deleteDisaster(id)) {
            crow::json::wvalue resp;
            resp["success"] = true;
            resp["message"] = "Disaster deleted successfully.";
            res.set_header("Content-Type", "application/json");
            res.write(resp.dump());
        } else {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Disaster not found or could not be deleted.";
            res.write(err.dump());
        }
        res.end();
    });

    // ----------------------------------------------------
    // Resources Inventory REST API
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/resources")
    .methods("GET"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto resources = db->getAllResources();
        std::vector<crow::json::wvalue> list;
        for (const auto& r : resources) {
            list.push_back(r.toCrowJson());
        }
        crow::json::wvalue result;
        result["resources"] = std::move(list);
        res.set_header("Content-Type", "application/json");
        res.write(result.dump());
        res.end();
    });

    CROW_ROUTE(app, "/api/resources/<int>")
    .methods("GET"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        auto item = db->getResourceById(id);
        if (!item) {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Resource not found.";
            res.write(err.dump());
        } else {
            res.set_header("Content-Type", "application/json");
            res.write(item->toCrowJson().dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/resources")
    .methods("POST"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid JSON payload.";
            res.write(err.dump());
            res.end();
            return;
        }

        std::string name = getJsonString(body, "name");
        std::string category = getJsonString(body, "category", "General");
        int quantity = getJsonInt(body, "quantity", 0);
        std::string unit = getJsonString(body, "unit", "units");
        std::string location = getJsonString(body, "storageLocation");

        if (name.empty()) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Resource name is required.";
            res.write(err.dump());
            res.end();
            return;
        }

        if (quantity < 0) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Resource quantity cannot be negative.";
            res.write(err.dump());
            res.end();
            return;
        }

        try {
            Resource r(0, name, category, quantity, unit, location, "");
            int id = db->addResource(r);
            if (id <= 0) {
                res.code = 500;
                crow::json::wvalue err;
                err["error"] = "Failed to add resource to database.";
                res.write(err.dump());
            } else {
                res.code = 201;
                r.setId(id);
                res.set_header("Content-Type", "application/json");
                res.write(r.toCrowJson().dump());
            }
        } catch (const std::exception& ex) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = ex.what();
            res.write(err.dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/resources/<int>")
    .methods("PUT"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        auto existing = db->getResourceById(id);
        if (!existing) {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Resource not found.";
            res.write(err.dump());
            res.end();
            return;
        }

        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid JSON payload.";
            res.write(err.dump());
            res.end();
            return;
        }

        std::string name = getJsonString(body, "name", existing->getName());
        std::string category = getJsonString(body, "category", existing->getCategory());
        int quantity = body.has("quantity") ? getJsonInt(body, "quantity") : existing->getQuantity();
        std::string unit = getJsonString(body, "unit", existing->getUnit());
        std::string location = getJsonString(body, "storageLocation", existing->getStorageLocation());

        if (quantity < 0) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Resource quantity cannot be negative.";
            res.write(err.dump());
            res.end();
            return;
        }

        try {
            Resource updated(id, name, category, quantity, unit, location, existing->getCreatedAt());
            if (db->updateResource(updated)) {
                res.set_header("Content-Type", "application/json");
                res.write(updated.toCrowJson().dump());
            } else {
                res.code = 500;
                crow::json::wvalue err;
                err["error"] = "Failed to update resource.";
                res.write(err.dump());
            }
        } catch (const std::exception& ex) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = ex.what();
            res.write(err.dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/resources/<int>")
    .methods("DELETE"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        if (db->deleteResource(id)) {
            crow::json::wvalue resp;
            resp["success"] = true;
            resp["message"] = "Resource deleted successfully.";
            res.set_header("Content-Type", "application/json");
            res.write(resp.dump());
        } else {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Resource not found.";
            res.write(err.dump());
        }
        res.end();
    });

    // ----------------------------------------------------
    // Relief Requests REST API
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/requests")
    .methods("GET"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto requests = db->getAllReliefRequests();
        std::vector<crow::json::wvalue> list;
        for (const auto& r : requests) {
            list.push_back(r.toCrowJson());
        }
        crow::json::wvalue result;
        result["requests"] = std::move(list);
        res.set_header("Content-Type", "application/json");
        res.write(result.dump());
        res.end();
    });

    CROW_ROUTE(app, "/api/requests")
    .methods("POST"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid JSON payload.";
            res.write(err.dump());
            res.end();
            return;
        }

        std::string reqName = getJsonString(body, "requesterName");
        std::string location = getJsonString(body, "location");
        std::string resource = getJsonString(body, "requiredResource");
        int quantity = getJsonInt(body, "quantity", 1);
        std::string urgency = getJsonString(body, "urgency", "Medium");
        std::string status = getJsonString(body, "status", "Pending");
        std::string contact = getJsonString(body, "contactInfo");

        if (reqName.empty() || resource.empty()) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Requester name and required resource are required.";
            res.write(err.dump());
            res.end();
            return;
        }

        try {
            ReliefRequest r(0, reqName, location, resource, quantity, urgency, status, contact, "");
            int id = db->addReliefRequest(r);
            if (id <= 0) {
                res.code = 500;
                crow::json::wvalue err;
                err["error"] = "Failed to create relief request.";
                res.write(err.dump());
            } else {
                res.code = 201;
                r.setId(id);
                res.set_header("Content-Type", "application/json");
                res.write(r.toCrowJson().dump());
            }
        } catch (const std::exception& ex) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = ex.what();
            res.write(err.dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/requests/<int>/status")
    .methods("PUT"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        auto body = crow::json::load(req.body);
        if (!body || !body.has("status")) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Status field is required.";
            res.write(err.dump());
            res.end();
            return;
        }

        std::string status = getJsonString(body, "status");
        if (!ReliefRequest::isValidStatus(status)) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid status value.";
            res.write(err.dump());
            res.end();
            return;
        }

        if (db->updateReliefRequestStatus(id, status)) {
            crow::json::wvalue resp;
            resp["success"] = true;
            resp["message"] = "Request status updated successfully.";
            res.set_header("Content-Type", "application/json");
            res.write(resp.dump());
        } else {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Request not found.";
            res.write(err.dump());
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/requests/<int>")
    .methods("DELETE"_method)
    ([db](const crow::request& req, crow::response& res, int id) {
        setCorsHeaders(res);
        if (db->deleteReliefRequest(id)) {
            crow::json::wvalue resp;
            resp["success"] = true;
            resp["message"] = "Relief request deleted.";
            res.set_header("Content-Type", "application/json");
            res.write(resp.dump());
        } else {
            res.code = 404;
            crow::json::wvalue err;
            err["error"] = "Relief request not found.";
            res.write(err.dump());
        }
        res.end();
    });

    // ----------------------------------------------------
    // Distributions REST API
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/distributions")
    .methods("GET"_method)
    ([db](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto list = db->getAllDistributions();
        std::vector<crow::json::wvalue> items;
        for (const auto& d : list) {
            items.push_back(d.toCrowJson());
        }
        crow::json::wvalue result;
        result["distributions"] = std::move(items);
        res.set_header("Content-Type", "application/json");
        res.write(result.dump());
        res.end();
    });

    CROW_ROUTE(app, "/api/distributions")
    .methods("POST"_method)
    ([&reliefSystem](const crow::request& req, crow::response& res) {
        setCorsHeaders(res);
        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = "Invalid JSON payload.";
            res.write(err.dump());
            res.end();
            return;
        }

        int resourceId = getJsonInt(body, "resourceId");
        int disasterId = getJsonInt(body, "disasterId");
        int quantity = getJsonInt(body, "quantity");
        std::string dest = getJsonString(body, "destination");
        int requestId = getJsonInt(body, "requestId");
        std::string notes = getJsonString(body, "notes");

        auto result = reliefSystem.allocateResource(resourceId, disasterId, quantity, dest, requestId, notes);
        if (!result.success) {
            res.code = 400;
            crow::json::wvalue err;
            err["error"] = result.message;
            res.set_header("Content-Type", "application/json");
            res.write(err.dump());
        } else {
            res.code = 201;
            crow::json::wvalue ok;
            ok["success"] = true;
            ok["message"] = result.message;
            ok["distributionId"] = result.distributionId;
            ok["remainingStock"] = result.remainingStock;
            res.set_header("Content-Type", "application/json");
            res.write(ok.dump());
        }
        res.end();
    });

    std::cout << ">> Server running at: http://localhost:" << port << std::endl;
    std::cout << ">> Open your browser and navigate to http://localhost:" << port << std::endl;

    app.port(port).multithreaded().run();
    return 0;
}
