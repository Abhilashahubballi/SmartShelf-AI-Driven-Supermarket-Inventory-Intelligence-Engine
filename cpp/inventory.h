#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <ctime>

struct Item {
    int id;
    std::string name;
    std::string category;
    int quantity;
    double price;
    int lowStockThreshold;
    std::string expiryDate;   // YYYY-MM-DD
    std::string supplier;
    int reorderQty;
};

class Inventory {
private:
    std::vector<Item> items;
    int nextId = 1;
    std::string dataFile = "../data/stock.csv";

    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end   = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

public:
    Inventory() { loadFromCSV(); }

    void loadFromCSV() {
        items.clear();
        std::ifstream file(dataFile);
        if (!file.is_open()) {
            seedDefaultData();
            return;
        }
        std::string line;
        std::getline(file, line); // skip header
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string tok;
            Item item;
            std::getline(ss, tok, ','); item.id            = std::stoi(trim(tok));
            std::getline(ss, tok, ','); item.name          = trim(tok);
            std::getline(ss, tok, ','); item.category      = trim(tok);
            std::getline(ss, tok, ','); item.quantity      = std::stoi(trim(tok));
            std::getline(ss, tok, ','); item.price         = std::stod(trim(tok));
            std::getline(ss, tok, ','); item.lowStockThreshold = std::stoi(trim(tok));
            std::getline(ss, tok, ','); item.expiryDate    = trim(tok);
            std::getline(ss, tok, ','); item.supplier      = trim(tok);
            std::getline(ss, tok, ','); item.reorderQty    = std::stoi(trim(tok));
            items.push_back(item);
            if (item.id >= nextId) nextId = item.id + 1;
        }
    }

    void saveToCSV() {
        std::ofstream file(dataFile);
        file << "id,name,category,quantity,price,low_stock_threshold,expiry_date,supplier,reorder_qty\n";
        for (auto& item : items) {
            file << item.id << "," << item.name << "," << item.category << ","
                 << item.quantity << "," << std::fixed << std::setprecision(2) << item.price << ","
                 << item.lowStockThreshold << "," << item.expiryDate << ","
                 << item.supplier << "," << item.reorderQty << "\n";
        }
    }

    void seedDefaultData() {
        items = {
            {1,  "Basmati Rice",     "Grains",      45, 85.00,  10, "2025-12-01", "AgroSupplies",   50},
            {2,  "Whole Wheat Flour","Grains",       3, 45.00,   5, "2025-08-15", "AgroSupplies",   30},
            {3,  "Toor Dal",         "Pulses",      12, 120.00,  8, "2025-11-20", "DalTraders",     25},
            {4,  "Sunflower Oil",    "Oils",         2, 180.00,  5, "2026-03-10", "OilMart",        15},
            {5,  "Full Cream Milk",  "Dairy",        8, 28.00,  10, "2025-06-18", "MilkFresh",      50},
            {6,  "Butter",           "Dairy",        1, 55.00,   3, "2025-06-20", "MilkFresh",      10},
            {7,  "Tomatoes",         "Vegetables",  20, 30.00,  10, "2025-06-15", "FarmFresh",      30},
            {8,  "Onions",           "Vegetables",   4, 25.00,   8, "2025-07-01", "FarmFresh",      25},
            {9,  "Potatoes",         "Vegetables",  35, 20.00,  10, "2025-07-15", "FarmFresh",      30},
            {10, "Sugar",            "Condiments",   6, 42.00,   5, "2026-01-01", "SweetSupplies",  20},
            {11, "Salt",             "Condiments",  50, 18.00,  10, "2026-06-01", "SweetSupplies",  20},
            {12, "Turmeric Powder",  "Spices",       2, 65.00,   3, "2025-10-01", "SpiceWorld",     10},
            {13, "Coriander Powder", "Spices",       7, 55.00,   3, "2025-10-15", "SpiceWorld",     10},
            {14, "Eggs (tray/30)",   "Poultry",      5, 180.00,  4, "2025-06-17", "PoultryFarm",    10},
            {15, "Bread",            "Bakery",       3, 40.00,   5, "2025-06-14", "BakeryHub",      15},
        };
        nextId = 16;
        saveToCSV();
    }

    // CRUD
    void addItem(Item item) {
        item.id = nextId++;
        items.push_back(item);
        saveToCSV();
    }

    bool updateItem(int id, Item updated) {
        for (auto& item : items) {
            if (item.id == id) {
                updated.id = id;
                item = updated;
                saveToCSV();
                return true;
            }
        }
        return false;
    }

    bool deleteItem(int id) {
        auto it = std::remove_if(items.begin(), items.end(),
            [id](const Item& i) { return i.id == id; });
        if (it != items.end()) {
            items.erase(it, items.end());
            saveToCSV();
            return true;
        }
        return false;
    }

    bool updateQuantity(int id, int newQty) {
        for (auto& item : items) {
            if (item.id == id) {
                item.quantity = newQty;
                saveToCSV();
                return true;
            }
        }
        return false;
    }

    // Getters
    std::vector<Item>& getAll() { return items; }

    Item* findById(int id) {
        for (auto& item : items)
            if (item.id == id) return &item;
        return nullptr;
    }

    std::vector<Item> searchByName(const std::string& query) {
        std::vector<Item> result;
        std::string q = query;
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
        for (auto& item : items) {
            std::string name = item.name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(q) != std::string::npos)
                result.push_back(item);
        }
        return result;
    }

    std::vector<Item> filterByCategory(const std::string& cat) {
        std::vector<Item> result;
        for (auto& item : items)
            if (item.category == cat) result.push_back(item);
        return result;
    }

    std::vector<Item> getLowStockItems() {
        std::vector<Item> result;
        for (auto& item : items)
            if (item.quantity <= item.lowStockThreshold)
                result.push_back(item);
        return result;
    }

    std::vector<Item> getCriticalItems() {
        std::vector<Item> result;
        for (auto& item : items)
            if (item.quantity <= 3)
                result.push_back(item);
        return result;
    }

    std::vector<Item> getExpiringItems(int withinDays = 7) {
        std::vector<Item> result;
        time_t now = time(0);
        struct tm* t = localtime(&now);
        char buf[11];
        strftime(buf, sizeof(buf), "%Y-%m-%d", t);
        std::string today(buf);

        for (auto& item : items) {
            if (item.expiryDate <= today) {
                result.push_back(item);
            } else {
                // simple date diff check (days)
                int dy = std::stoi(item.expiryDate.substr(0,4))  - std::stoi(today.substr(0,4));
                int dm = std::stoi(item.expiryDate.substr(5,2))  - std::stoi(today.substr(5,2));
                int dd = std::stoi(item.expiryDate.substr(8,2))  - std::stoi(today.substr(8,2));
                int totalDays = dy*365 + dm*30 + dd;
                if (totalDays <= withinDays) result.push_back(item);
            }
        }
        return result;
    }

    std::map<std::string, int> getCategoryCount() {
        std::map<std::string, int> counts;
        for (auto& item : items) counts[item.category]++;
        return counts;
    }

    double getTotalInventoryValue() {
        double total = 0;
        for (auto& item : items) total += item.quantity * item.price;
        return total;
    }
};
