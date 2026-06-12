#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include "inventory.h"

namespace Utils {

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader(const std::string& title) {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║";
    int pad = (54 - (int)title.size()) / 2;
    for (int i = 0; i < pad; i++) std::cout << " ";
    std::cout << title;
    for (int i = 0; i < 54 - pad - (int)title.size(); i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n\n";
}

void printDivider() {
    std::cout << "  ──────────────────────────────────────────────────────\n";
}

void pause() {
    std::cout << "\n  Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

void printInventoryTable(const std::vector<Item>& items) {
    if (items.empty()) {
        std::cout << "  No items found.\n";
        return;
    }
    std::cout << "\n";
    std::cout << "  " << std::left
              << std::setw(5)  << "ID"
              << std::setw(22) << "Name"
              << std::setw(14) << "Category"
              << std::setw(8)  << "Qty"
              << std::setw(10) << "Price"
              << std::setw(8)  << "Thresh"
              << std::setw(12) << "Expiry"
              << std::setw(16) << "Supplier" << "\n";
    printDivider();
    for (auto& item : items) {
        std::string qtyStr = std::to_string(item.quantity);
        if (item.quantity <= 3)         qtyStr += " ⚠️ CRITICAL";
        else if (item.quantity <= item.lowStockThreshold) qtyStr += " ⚠️ LOW";

        std::cout << "  " << std::left
                  << std::setw(5)  << item.id
                  << std::setw(22) << item.name
                  << std::setw(14) << item.category
                  << std::setw(8)  << qtyStr
                  << "₹" << std::setw(9) << std::fixed << std::setprecision(2) << item.price
                  << std::setw(8)  << item.lowStockThreshold
                  << std::setw(12) << item.expiryDate
                  << std::setw(16) << item.supplier << "\n";
    }
    std::cout << "\n";
}

std::string getInput(const std::string& prompt) {
    std::cout << "  " << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int getIntInput(const std::string& prompt) {
    while (true) {
        std::cout << "  " << prompt;
        std::string s;
        std::getline(std::cin, s);
        try {
            return std::stoi(s);
        } catch (...) {
            std::cout << "  ⚠️  Please enter a valid number.\n";
        }
    }
}

double getDoubleInput(const std::string& prompt) {
    while (true) {
        std::cout << "  " << prompt;
        std::string s;
        std::getline(std::cin, s);
        try {
            return std::stod(s);
        } catch (...) {
            std::cout << "  ⚠️  Please enter a valid number.\n";
        }
    }
}

} // namespace Utils
