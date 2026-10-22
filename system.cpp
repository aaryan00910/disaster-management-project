#include "system.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <map>

DisasterReliefSystem::DisasterReliefSystem(std::shared_ptr<Database> db)
    : m_db(std::move(db)) {}

DisasterReliefSystem::AllocationResult DisasterReliefSystem::allocateResource(
    int resourceId, int disasterId, int quantity,
    const std::string& destination, int requestId,
    const std::string& notes) {

    AllocationResult res;
    res.success = false;
    res.distributionId = 0;
    res.remainingStock = 0;

    if (quantity <= 0) {
        res.message = "Allocation quantity must be greater than zero.";
        return res;
    }

    auto resource = m_db->getResourceById(resourceId);
    if (!resource) {
        res.message = "Resource with ID " + std::to_string(resourceId) + " not found.";
        return res;
    }

    auto disaster = m_db->getDisasterById(disasterId);
    if (!disaster) {
        res.message = "Disaster with ID " + std::to_string(disasterId) + " not found.";
        return res;
    }

    if (!resource->hasSufficientStock(quantity)) {
        res.message = "Insufficient inventory: available " + std::to_string(resource->getQuantity()) +
                      " " + resource->getUnit() + ", but requested " + std::to_string(quantity) + ".";
        return res;
    }

    // Begin atomic transaction
    m_db->beginTransaction();

    try {
        // Deduct inventory
        resource->deductStock(quantity);
        if (!m_db->updateResourceQuantity(resourceId, resource->getQuantity())) {
            m_db->rollbackTransaction();
            res.message = "Failed to update resource inventory in database.";
            return res;
        }

        // Get current date
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        std::string today = oss.str();

        std::string effectiveDestination = destination;
        if (effectiveDestination.empty()) {
            effectiveDestination = disaster->getLocation();
        }

        Distribution dist(0, resourceId, resource->getName(),
                          disasterId, disaster->getName(),
                          requestId, quantity, effectiveDestination,
                          today, "Dispatched", notes, "");

        int distId = m_db->addDistribution(dist);
        if (distId <= 0) {
            m_db->rollbackTransaction();
            res.message = "Failed to log distribution record in database.";
            return res;
        }

        // If this distribution fulfills a relief request, update request status
        if (requestId > 0) {
            auto req = m_db->getReliefRequestById(requestId);
            if (req) {
                if (quantity >= req->getQuantity()) {
                    m_db->updateReliefRequestStatus(requestId, "Completed");
                } else {
                    m_db->updateReliefRequestStatus(requestId, "In Progress");
                }
            }
        }

        m_db->commitTransaction();

        res.success = true;
        res.distributionId = distId;
        res.remainingStock = resource->getQuantity();
        res.message = "Successfully allocated " + std::to_string(quantity) + " " +
                      resource->getUnit() + " of " + resource->getName() + ".";
        return res;
    } catch (const std::exception& e) {
        m_db->rollbackTransaction();
        res.message = std::string("Transaction exception: ") + e.what();
        return res;
    }
}

crow::json::wvalue DisasterReliefSystem::getDashboardSummary() {
    crow::json::wvalue summary;

    auto disasters = m_db->getAllDisasters();
    auto resources = m_db->getAllResources();
    auto requests = m_db->getAllReliefRequests();
    auto distributions = m_db->getAllDistributions();

    int activeDisasters = 0;
    int criticalDisasters = 0;
    for (const auto& d : disasters) {
        if (d.getStatus() == "Active") activeDisasters++;
        if (d.getSeverity() == "Critical") criticalDisasters++;
    }

    int totalResourcesInStock = 0;
    int lowStockAlerts = 0;
    for (const auto& r : resources) {
        totalResourcesInStock += r.getQuantity();
        if (r.getQuantity() < 50) lowStockAlerts++;
    }

    int pendingRequests = 0;
    int approvedRequests = 0;
    int completedRequests = 0;
    for (const auto& req : requests) {
        if (req.getStatus() == "Pending") pendingRequests++;
        else if (req.getStatus() == "Approved" || req.getStatus() == "In Progress") approvedRequests++;
        else if (req.getStatus() == "Completed") completedRequests++;
    }

    int totalItemsDistributed = 0;
    for (const auto& dist : distributions) {
        totalItemsDistributed += dist.getQuantity();
    }

    summary["totalDisasters"] = static_cast<int>(disasters.size());
    summary["activeDisasters"] = activeDisasters;
    summary["criticalDisasters"] = criticalDisasters;

    summary["totalResourceTypes"] = static_cast<int>(resources.size());
    summary["totalStockItems"] = totalResourcesInStock;
    summary["lowStockAlerts"] = lowStockAlerts;

    summary["totalRequests"] = static_cast<int>(requests.size());
    summary["pendingRequests"] = pendingRequests;
    summary["activeRequests"] = approvedRequests;
    summary["completedRequests"] = completedRequests;

    summary["totalDistributions"] = static_cast<int>(distributions.size());
    summary["totalItemsDistributed"] = totalItemsDistributed;

    // Recent 5 disasters
    std::vector<crow::json::wvalue> recentDisasters;
    int disLimit = std::min(5, static_cast<int>(disasters.size()));
    for (int i = 0; i < disLimit; ++i) {
        recentDisasters.push_back(disasters[i].toCrowJson());
    }
    summary["recentDisasters"] = std::move(recentDisasters);

    // Recent 5 distributions
    std::vector<crow::json::wvalue> recentDistributions;
    int distLimit = std::min(5, static_cast<int>(distributions.size()));
    for (int i = 0; i < distLimit; ++i) {
        recentDistributions.push_back(distributions[i].toCrowJson());
    }
    summary["recentDistributions"] = std::move(recentDistributions);

    return summary;
}

crow::json::wvalue DisasterReliefSystem::getDetailedReport() {
    crow::json::wvalue report;

    auto disasters = m_db->getAllDisasters();
    auto resources = m_db->getAllResources();
    auto requests = m_db->getAllReliefRequests();
    auto distributions = m_db->getAllDistributions();

    // Severity Breakdown
    std::map<std::string, int> severityCount;
    for (const auto& d : disasters) {
        severityCount[d.getSeverity()]++;
    }
    crow::json::wvalue sevJson;
    for (const auto& [sev, count] : severityCount) {
        sevJson[sev] = count;
    }
    report["severityBreakdown"] = std::move(sevJson);

    // Category Breakdown for Resources
    std::map<std::string, int> categoryStock;
    for (const auto& r : resources) {
        categoryStock[r.getCategory()] += r.getQuantity();
    }
    crow::json::wvalue catJson;
    for (const auto& [cat, qty] : categoryStock) {
        catJson[cat] = qty;
    }
    report["categoryStock"] = std::move(catJson);

    // Request Status Breakdown
    std::map<std::string, int> requestStatus;
    for (const auto& req : requests) {
        requestStatus[req.getStatus()]++;
    }
    crow::json::wvalue statusJson;
    for (const auto& [st, count] : requestStatus) {
        statusJson[st] = count;
    }
    report["requestStatus"] = std::move(statusJson);

    // All lists for table printing
    std::vector<crow::json::wvalue> disList;
    for (const auto& d : disasters) disList.push_back(d.toCrowJson());
    report["disasters"] = std::move(disList);

    std::vector<crow::json::wvalue> resList;
    for (const auto& r : resources) resList.push_back(r.toCrowJson());
    report["resources"] = std::move(resList);

    std::vector<crow::json::wvalue> reqList;
    for (const auto& rq : requests) reqList.push_back(rq.toCrowJson());
    report["requests"] = std::move(reqList);

    std::vector<crow::json::wvalue> distList;
    for (const auto& dt : distributions) distList.push_back(dt.toCrowJson());
    report["distributions"] = std::move(distList);

    return report;
}
