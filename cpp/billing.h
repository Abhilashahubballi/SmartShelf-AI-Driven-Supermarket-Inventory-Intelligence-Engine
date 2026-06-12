#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

struct SaleItem {
    int itemId;
    std::string itemName;
    int quantity;
    double unitPrice;
    double total;
};

struct Transaction {
    int id;
    std::string datetime;
    std::string cashier;
    std::vector<SaleItem> items;
    double grandTotal;
    std::string paymentMode;
};

class BillingManager {
private:
    std::vector<Transaction> transactions;
    int nextId = 1;
    std::string dataFile = "../data/sales.csv";

    std::string currentDateTime() {
        time_t now = time(0);
        struct tm* t = localtime(&now);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
        return std::string(buf);
    }

public:
    BillingManager() { loadFromCSV(); }

    void loadFromCSV() {
        transactions.clear();
        std::ifstream file(dataFile);
        if (!file.is_open()) return;
        std::string line;
        std::getline(file, line); // header
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string tok;
            Transaction t;
            std::getline(ss, tok, ','); t.id          = std::stoi(tok);
            std::getline(ss, tok, ','); t.datetime     = tok;
            std::getline(ss, tok, ','); t.cashier      = tok;
            std::getline(ss, tok, ','); t.grandTotal   = std::stod(tok);
            std::getline(ss, tok, ','); t.paymentMode  = tok;
            transactions.push_back(t);
            if (t.id >= nextId) nextId = t.id + 1;
        }
    }

    void saveTransaction(const Transaction& t) {
        std::ofstream file(dataFile, std::ios::app);
        if (file.tellp() == 0)
            file << "id,datetime,cashier,grand_total,payment_mode\n";
        file << t.id << "," << t.datetime << "," << t.cashier << ","
             << std::fixed << std::setprecision(2) << t.grandTotal << ","
             << t.paymentMode << "\n";
    }

    Transaction createTransaction(const std::vector<SaleItem>& items,
                                   const std::string& cashier,
                                   const std::string& paymentMode) {
        Transaction t;
        t.id          = nextId++;
        t.datetime    = currentDateTime();
        t.cashier     = cashier;
        t.items       = items;
        t.paymentMode = paymentMode;
        t.grandTotal  = 0;
        for (auto& si : items) t.grandTotal += si.total;
        transactions.push_back(t);
        saveTransaction(t);
        return t;
    }

    void printInvoice(const Transaction& t) {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════╗\n";
        std::cout << "  ║          SMARTSHELF SUPERMARKET          ║\n";
        std::cout << "  ║         MG Road, Bengaluru - 560001      ║\n";
        std::cout << "  ╠══════════════════════════════════════════╣\n";
        std::cout << "  Invoice #: " << std::setw(5) << t.id
                  << "     Date: " << t.datetime.substr(0,10) << "\n";
        std::cout << "  Time: " << t.datetime.substr(11)
                  << "        Cashier: " << t.cashier << "\n";
        std::cout << "  ──────────────────────────────────────────\n";
        std::cout << "  " << std::left << std::setw(20) << "Item"
                  << std::setw(5) << "Qty"
                  << std::setw(10) << "Price"
                  << std::setw(10) << "Total" << "\n";
        std::cout << "  ──────────────────────────────────────────\n";
        for (auto& si : t.items) {
            std::cout << "  " << std::left << std::setw(20) << si.itemName
                      << std::setw(5) << si.quantity
                      << "₹" << std::setw(9) << std::fixed << std::setprecision(2) << si.unitPrice
                      << "₹" << std::fixed << std::setprecision(2) << si.total << "\n";
        }
        std::cout << "  ──────────────────────────────────────────\n";
        std::cout << "  " << std::right << std::setw(35) << "GRAND TOTAL: ₹"
                  << std::fixed << std::setprecision(2) << t.grandTotal << "\n";
        std::cout << "  Payment: " << t.paymentMode << "\n";
        std::cout << "  ╠══════════════════════════════════════════╣\n";
        std::cout << "  ║     Thank you for shopping with us!      ║\n";
        std::cout << "  ╚══════════════════════════════════════════╝\n\n";
    }

    std::vector<Transaction>& getAll() { return transactions; }

    double getTotalRevenue() {
        double total = 0;
        for (auto& t : transactions) total += t.grandTotal;
        return total;
    }
};
