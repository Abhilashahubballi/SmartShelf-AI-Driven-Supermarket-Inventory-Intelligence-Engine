#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

struct OrderItem {
    int itemId;
    std::string itemName;
    std::string category;
    int currentQty;
    int orderedQty;
    double unitPrice;
    std::string supplier;
    bool aiSuggested;
};

struct VendorOrder {
    int orderId;
    std::string createdDate;
    std::string deliveryDate;
    std::string approvedBy;
    std::string status; // PENDING, APPROVED, DELIVERED
    std::vector<OrderItem> items;
    double totalValue;
};

class OrderManager {
private:
    std::vector<VendorOrder> orders;
    int nextId = 1;
    std::string dataFile = "../data/orders.csv";

    std::string today() {
        time_t now = time(0);
        struct tm* t = localtime(&now);
        char buf[11];
        strftime(buf, sizeof(buf), "%Y-%m-%d", t);
        return std::string(buf);
    }

public:
    OrderManager() { loadFromCSV(); }

    void loadFromCSV() {
        orders.clear();
        std::ifstream file(dataFile);
        if (!file.is_open()) return;
        std::string line;
        std::getline(file, line);
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string tok;
            VendorOrder o;
            std::getline(ss, tok, ','); o.orderId      = std::stoi(tok);
            std::getline(ss, tok, ','); o.createdDate  = tok;
            std::getline(ss, tok, ','); o.deliveryDate = tok;
            std::getline(ss, tok, ','); o.approvedBy   = tok;
            std::getline(ss, tok, ','); o.status       = tok;
            std::getline(ss, tok, ','); o.totalValue   = std::stod(tok);
            orders.push_back(o);
            if (o.orderId >= nextId) nextId = o.orderId + 1;
        }
    }

    void saveToCSV() {
        std::ofstream file(dataFile);
        file << "order_id,created_date,delivery_date,approved_by,status,total_value\n";
        for (auto& o : orders) {
            file << o.orderId << "," << o.createdDate << "," << o.deliveryDate << ","
                 << o.approvedBy << "," << o.status << ","
                 << std::fixed << std::setprecision(2) << o.totalValue << "\n";
        }
    }

    VendorOrder createOrder(std::vector<OrderItem> items,
                             const std::string& deliveryDate,
                             const std::string& approvedBy) {
        VendorOrder order;
        order.orderId      = nextId++;
        order.createdDate  = today();
        order.deliveryDate = deliveryDate;
        order.approvedBy   = approvedBy;
        order.status       = "APPROVED";
        order.items        = items;
        order.totalValue   = 0;
        for (auto& i : items)
            order.totalValue += i.orderedQty * i.unitPrice;
        orders.push_back(order);
        saveToCSV();
        exportOrderCSV(order);
        return order;
    }

    void exportOrderCSV(const VendorOrder& order) {
        std::string fname = "../output/vendor_order_" + std::to_string(order.orderId) + ".csv";
        std::ofstream f(fname);
        f << "item_id,item_name,category,current_qty,ordered_qty,unit_price,total,supplier,ai_suggested\n";
        for (auto& item : order.items) {
            f << item.itemId << "," << item.itemName << "," << item.category << ","
              << item.currentQty << "," << item.orderedQty << ","
              << std::fixed << std::setprecision(2) << item.unitPrice << ","
              << item.orderedQty * item.unitPrice << ","
              << item.supplier << "," << (item.aiSuggested ? "Yes" : "No") << "\n";
        }
        std::cout << "\n  ✅ Order CSV exported → " << fname << "\n";
    }

    void printOrderSummary(const VendorOrder& order) {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
        std::cout << "  ║             SMARTSHELF — VENDOR ORDER                ║\n";
        std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
        std::cout << "  Order ID    : #" << order.orderId << "\n";
        std::cout << "  Created     : " << order.createdDate << "\n";
        std::cout << "  Delivery By : " << order.deliveryDate << "\n";
        std::cout << "  Approved By : " << order.approvedBy << "\n";
        std::cout << "  Status      : " << order.status << "\n";
        std::cout << "  ──────────────────────────────────────────────────────\n";
        std::cout << "  " << std::left
                  << std::setw(20) << "Item"
                  << std::setw(8)  << "Cur.Qty"
                  << std::setw(8)  << "Order"
                  << std::setw(10) << "Price"
                  << std::setw(10) << "Total"
                  << std::setw(6)  << "AI?" << "\n";
        std::cout << "  ──────────────────────────────────────────────────────\n";
        for (auto& item : order.items) {
            std::cout << "  " << std::left
                      << std::setw(20) << item.itemName
                      << std::setw(8)  << item.currentQty
                      << std::setw(8)  << item.orderedQty
                      << "₹" << std::setw(9) << std::fixed << std::setprecision(2) << item.unitPrice
                      << "₹" << std::setw(9) << item.orderedQty * item.unitPrice
                      << std::setw(6)  << (item.aiSuggested ? "✓" : "✗") << "\n";
        }
        std::cout << "  ──────────────────────────────────────────────────────\n";
        std::cout << "  " << std::right << std::setw(46) << "TOTAL ORDER VALUE: ₹"
                  << std::fixed << std::setprecision(2) << order.totalValue << "\n";
        std::cout << "  ╚══════════════════════════════════════════════════════╝\n\n";
    }

    bool markDelivered(int orderId) {
        for (auto& o : orders) {
            if (o.orderId == orderId) {
                o.status = "DELIVERED";
                saveToCSV();
                return true;
            }
        }
        return false;
    }

    std::vector<VendorOrder>& getAll() { return orders; }

    std::vector<VendorOrder> getPending() {
        std::vector<VendorOrder> result;
        for (auto& o : orders)
            if (o.status == "APPROVED") result.push_back(o);
        return result;
    }
};
