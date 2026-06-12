#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <limits>

#include "auth.h"
#include "inventory.h"
#include "billing.h"
#include "orders.h"
#include "utils.h"

Auth          auth;
Inventory     inventory;
BillingManager billing;
OrderManager  orderMgr;

// ─── Forward declarations ───────────────────────────────────────────────────
void mainMenu();
void inventoryMenu();
void billingMenu();
void aiRestockMenu();
void ordersMenu();
void reportsMenu();

// ─── LOGIN ───────────────────────────────────────────────────────────────────
bool loginScreen() {
    Utils::printHeader("SMARTSHELF — Login");
    std::cout << "  Credentials:\n";
    std::cout << "  manager / manager123  (full access)\n";
    std::cout << "  cashier / cashier123  (billing + view)\n";
    std::cout << "  viewer  / viewer123   (view only)\n\n";

    std::string user = Utils::getInput("Username: ");
    std::string pass = Utils::getInput("Password: ");

    if (auth.login(user, pass)) {
        std::cout << "\n  ✅ Welcome, " << auth.getUsername()
                  << " [" << auth.roleToString() << "]\n";
        Utils::pause();
        return true;
    }
    std::cout << "\n  ❌ Invalid credentials.\n";
    Utils::pause();
    return false;
}

// ─── MAIN MENU ───────────────────────────────────────────────────────────────
void mainMenu() {
    while (true) {
        Utils::clearScreen();
        Utils::printHeader("SMARTSHELF — AI Inventory Intelligence");
        std::cout << "  👤 Logged in as: " << auth.getUsername()
                  << " [" << auth.roleToString() << "]\n\n";

        // Quick alerts
        auto lowItems      = inventory.getLowStockItems();
        auto criticalItems = inventory.getCriticalItems();
        auto expiringItems = inventory.getExpiringItems(7);

        if (!criticalItems.empty())
            std::cout << "  🚨 CRITICAL: " << criticalItems.size() << " item(s) almost out of stock!\n";
        if (!lowItems.empty())
            std::cout << "  ⚠️  LOW STOCK: " << lowItems.size() << " item(s) below threshold\n";
        if (!expiringItems.empty())
            std::cout << "  📅 EXPIRING: " << expiringItems.size() << " item(s) expiring within 7 days\n";

        std::cout << "\n";
        std::cout << "  [1] 📦 Inventory Management\n";
        std::cout << "  [2] 🧾 Billing & Sales\n";
        std::cout << "  [3] 🤖 AI Restock Advisor\n";
        std::cout << "  [4] 📋 Vendor Orders\n";
        std::cout << "  [5] 📊 Reports & Dashboard\n";
        std::cout << "  [6] 🚪 Logout\n\n";

        int choice = Utils::getIntInput("Choose option: ");
        switch (choice) {
            case 1: inventoryMenu(); break;
            case 2: billingMenu();   break;
            case 3: aiRestockMenu(); break;
            case 4: ordersMenu();    break;
            case 5: reportsMenu();   break;
            case 6: auth.logout(); return;
            default: std::cout << "  Invalid option.\n";
        }
    }
}

// ─── INVENTORY MENU ──────────────────────────────────────────────────────────
void inventoryMenu() {
    while (true) {
        Utils::clearScreen();
        Utils::printHeader("Inventory Management");

        std::cout << "  [1] View All Items\n";
        std::cout << "  [2] Search Item\n";
        std::cout << "  [3] Filter by Category\n";
        std::cout << "  [4] View Low Stock Items\n";
        std::cout << "  [5] View Expiring Items\n";
        if (auth.hasPermission(Role::CASHIER)) {
            std::cout << "  [6] Update Item Quantity\n";
        }
        if (auth.hasPermission(Role::MANAGER)) {
            std::cout << "  [7] Add New Item\n";
            std::cout << "  [8] Edit Item\n";
            std::cout << "  [9] Delete Item\n";
        }
        std::cout << "  [0] Back\n\n";

        int choice = Utils::getIntInput("Choose option: ");

        if (choice == 0) break;

        else if (choice == 1) {
            Utils::clearScreen();
            Utils::printHeader("All Inventory Items");
            Utils::printInventoryTable(inventory.getAll());
            std::cout << "  Total items: " << inventory.getAll().size() << "\n";
            std::cout << "  Total inventory value: ₹"
                      << std::fixed << std::setprecision(2)
                      << inventory.getTotalInventoryValue() << "\n";
            Utils::pause();
        }

        else if (choice == 2) {
            std::string q = Utils::getInput("Search by name: ");
            auto results = inventory.searchByName(q);
            Utils::printInventoryTable(results);
            Utils::pause();
        }

        else if (choice == 3) {
            auto cats = inventory.getCategoryCount();
            std::cout << "\n  Categories:\n";
            for (auto& c : cats) std::cout << "  - " << c.first << "\n";
            std::string cat = Utils::getInput("\nEnter category: ");
            Utils::printInventoryTable(inventory.filterByCategory(cat));
            Utils::pause();
        }

        else if (choice == 4) {
            Utils::clearScreen();
            Utils::printHeader("Low Stock Items");
            Utils::printInventoryTable(inventory.getLowStockItems());
            Utils::pause();
        }

        else if (choice == 5) {
            Utils::clearScreen();
            Utils::printHeader("Items Expiring Within 7 Days");
            Utils::printInventoryTable(inventory.getExpiringItems(7));
            Utils::pause();
        }

        else if (choice == 6 && auth.hasPermission(Role::CASHIER)) {
            int id  = Utils::getIntInput("Item ID: ");
            int qty = Utils::getIntInput("New quantity: ");
            if (inventory.updateQuantity(id, qty))
                std::cout << "  ✅ Quantity updated.\n";
            else
                std::cout << "  ❌ Item not found.\n";
            Utils::pause();
        }

        else if (choice == 7 && auth.hasPermission(Role::MANAGER)) {
            Item item;
            item.name      = Utils::getInput("Name: ");
            item.category  = Utils::getInput("Category: ");
            item.quantity  = Utils::getIntInput("Quantity: ");
            item.price     = Utils::getDoubleInput("Price (₹): ");
            item.lowStockThreshold = Utils::getIntInput("Low stock threshold: ");
            item.expiryDate = Utils::getInput("Expiry date (YYYY-MM-DD): ");
            item.supplier   = Utils::getInput("Supplier: ");
            item.reorderQty = Utils::getIntInput("Reorder quantity: ");
            inventory.addItem(item);
            std::cout << "  ✅ Item added.\n";
            Utils::pause();
        }

        else if (choice == 8 && auth.hasPermission(Role::MANAGER)) {
            int id = Utils::getIntInput("Item ID to edit: ");
            Item* existing = inventory.findById(id);
            if (!existing) { std::cout << "  ❌ Not found.\n"; Utils::pause(); continue; }
            Item updated = *existing;
            std::cout << "  (Press Enter to keep current value)\n";
            std::string tmp;
            tmp = Utils::getInput("Name [" + updated.name + "]: ");
            if (!tmp.empty()) updated.name = tmp;
            tmp = Utils::getInput("Category [" + updated.category + "]: ");
            if (!tmp.empty()) updated.category = tmp;
            tmp = Utils::getInput("Supplier [" + updated.supplier + "]: ");
            if (!tmp.empty()) updated.supplier = tmp;
            tmp = Utils::getInput("Expiry [" + updated.expiryDate + "]: ");
            if (!tmp.empty()) updated.expiryDate = tmp;
            inventory.updateItem(id, updated);
            std::cout << "  ✅ Item updated.\n";
            Utils::pause();
        }

        else if (choice == 9 && auth.hasPermission(Role::MANAGER)) {
            int id = Utils::getIntInput("Item ID to delete: ");
            std::string confirm = Utils::getInput("Confirm delete? (yes/no): ");
            if (confirm == "yes" && inventory.deleteItem(id))
                std::cout << "  ✅ Item deleted.\n";
            else
                std::cout << "  ❌ Cancelled or not found.\n";
            Utils::pause();
        }
    }
}

// ─── BILLING MENU ────────────────────────────────────────────────────────────
void billingMenu() {
    if (!auth.hasPermission(Role::CASHIER)) {
        std::cout << "  ❌ Access denied.\n"; Utils::pause(); return;
    }
    while (true) {
        Utils::clearScreen();
        Utils::printHeader("Billing & Sales");
        std::cout << "  [1] New Sale\n";
        std::cout << "  [2] View Transaction History\n";
        std::cout << "  [0] Back\n\n";

        int choice = Utils::getIntInput("Choose option: ");
        if (choice == 0) break;

        else if (choice == 1) {
            Utils::printInventoryTable(inventory.getAll());
            std::vector<SaleItem> saleItems;
            std::cout << "  Add items to bill (enter 0 as ID to finish)\n\n";

            while (true) {
                int id = Utils::getIntInput("Item ID: ");
                if (id == 0) break;
                Item* item = inventory.findById(id);
                if (!item) { std::cout << "  ❌ Not found.\n"; continue; }
                int qty = Utils::getIntInput("Quantity: ");
                if (qty > item->quantity) {
                    std::cout << "  ❌ Insufficient stock (available: " << item->quantity << ")\n";
                    continue;
                }
                SaleItem si;
                si.itemId    = item->id;
                si.itemName  = item->name;
                si.quantity  = qty;
                si.unitPrice = item->price;
                si.total     = qty * item->price;
                saleItems.push_back(si);
                inventory.updateQuantity(id, item->quantity - qty);
                std::cout << "  ✅ Added: " << item->name << " x" << qty << "\n";
            }

            if (saleItems.empty()) { std::cout << "  No items added.\n"; Utils::pause(); continue; }

            std::string payment = Utils::getInput("Payment mode (Cash/Card/UPI): ");
            Transaction t = billing.createTransaction(saleItems, auth.getUsername(), payment);
            billing.printInvoice(t);
            Utils::pause();
        }

        else if (choice == 2) {
            Utils::clearScreen();
            Utils::printHeader("Transaction History");
            auto& txns = billing.getAll();
            if (txns.empty()) { std::cout << "  No transactions yet.\n"; }
            else {
                std::cout << "  " << std::left
                          << std::setw(6) << "ID"
                          << std::setw(22) << "DateTime"
                          << std::setw(12) << "Cashier"
                          << std::setw(12) << "Total"
                          << std::setw(10) << "Payment" << "\n";
                Utils::printDivider();
                for (auto& t : txns) {
                    std::cout << "  " << std::left
                              << std::setw(6)  << t.id
                              << std::setw(22) << t.datetime
                              << std::setw(12) << t.cashier
                              << "₹" << std::setw(11) << std::fixed << std::setprecision(2) << t.grandTotal
                              << std::setw(10) << t.paymentMode << "\n";
                }
                std::cout << "\n  Total Revenue: ₹"
                          << std::fixed << std::setprecision(2) << billing.getTotalRevenue() << "\n";
            }
            Utils::pause();
        }
    }
}

// ─── AI RESTOCK MENU ─────────────────────────────────────────────────────────
void aiRestockMenu() {
    if (!auth.hasPermission(Role::MANAGER)) {
        std::cout << "  ❌ Manager access required.\n"; Utils::pause(); return;
    }

    Utils::clearScreen();
    Utils::printHeader("AI Restock Advisor");

    std::cout << "  🤖 Running AI analysis...\n\n";

    // Build AI suggestion list from low stock + critical items
    auto lowItems = inventory.getLowStockItems();
    if (lowItems.empty()) {
        std::cout << "  ✅ All items are well stocked. No restock needed.\n";
        Utils::pause(); return;
    }

    // Display AI suggestions
    std::cout << "  📋 AI-Generated Restock Suggestions:\n\n";
    std::cout << "  " << std::left
              << std::setw(5)  << "No."
              << std::setw(22) << "Item"
              << std::setw(10) << "Cur.Qty"
              << std::setw(12) << "AI Suggest"
              << std::setw(10) << "Price"
              << std::setw(16) << "Supplier" << "\n";
    Utils::printDivider();

    std::vector<OrderItem> aiList;
    int idx = 1;
    for (auto& item : lowItems) {
        OrderItem oi;
        oi.itemId      = item.id;
        oi.itemName    = item.name;
        oi.category    = item.category;
        oi.currentQty  = item.quantity;
        oi.orderedQty  = item.reorderQty;
        oi.unitPrice   = item.price;
        oi.supplier    = item.supplier;
        oi.aiSuggested = true;
        aiList.push_back(oi);

        std::cout << "  " << std::left
                  << std::setw(5)  << idx++
                  << std::setw(22) << item.name
                  << std::setw(10) << item.quantity
                  << std::setw(12) << item.reorderQty
                  << "₹" << std::setw(9) << std::fixed << std::setprecision(2) << item.price
                  << std::setw(16) << item.supplier << "\n";
    }

    std::cout << "\n  ──────────────────────────────────────────────────────\n";
    std::cout << "  🤖 AI Summary:\n";
    std::cout << "  " << lowItems.size() << " item(s) need restocking.\n";

    // Count critical
    int critCount = 0;
    for (auto& i : lowItems) if (i.quantity <= 3) critCount++;
    if (critCount > 0)
        std::cout << "  🚨 " << critCount << " item(s) are CRITICALLY low (≤3 units)!\n";

    std::cout << "\n  ── Human Review Options ──────────────────────────────\n";
    std::cout << "  [1] ✅ Approve all AI suggestions as-is\n";
    std::cout << "  [2] ✏️  Edit quantities before approving\n";
    std::cout << "  [3] ➕ Add extra item to order\n";
    std::cout << "  [4] ❌ Remove item from order\n";
    std::cout << "  [5] 🚫 Cancel — do not place order\n\n";

    bool editing = true;
    while (editing) {
        int action = Utils::getIntInput("Choose action: ");

        if (action == 1) {
            editing = false;
        }
        else if (action == 2) {
            int no = Utils::getIntInput("Enter item No. to edit quantity: ");
            if (no >= 1 && no <= (int)aiList.size()) {
                int newQty = Utils::getIntInput("New order quantity: ");
                aiList[no-1].orderedQty  = newQty;
                aiList[no-1].aiSuggested = false;
                std::cout << "  ✅ Updated.\n";
            } else std::cout << "  Invalid number.\n";
        }
        else if (action == 3) {
            OrderItem extra;
            extra.itemId = Utils::getIntInput("Item ID to add: ");
            Item* it = inventory.findById(extra.itemId);
            if (!it) { std::cout << "  ❌ Not found.\n"; continue; }
            extra.itemName    = it->name;
            extra.category    = it->category;
            extra.currentQty  = it->quantity;
            extra.orderedQty  = Utils::getIntInput("Order quantity: ");
            extra.unitPrice   = it->price;
            extra.supplier    = it->supplier;
            extra.aiSuggested = false;
            aiList.push_back(extra);
            std::cout << "  ✅ " << extra.itemName << " added to order.\n";
        }
        else if (action == 4) {
            int no = Utils::getIntInput("Item No. to remove: ");
            if (no >= 1 && no <= (int)aiList.size()) {
                std::cout << "  ❌ Removed: " << aiList[no-1].itemName << "\n";
                aiList.erase(aiList.begin() + no - 1);
            } else std::cout << "  Invalid number.\n";
        }
        else if (action == 5) {
            std::cout << "  Order cancelled.\n";
            Utils::pause(); return;
        }
        else {
            editing = false;
        }

        if (action >= 2 && action <= 4) {
            // re-show list
            std::cout << "\n  Current order list:\n";
            std::cout << "  " << std::left
                      << std::setw(5)  << "No."
                      << std::setw(22) << "Item"
                      << std::setw(10) << "Cur.Qty"
                      << std::setw(12) << "Order Qty"
                      << std::setw(6)  << "AI?" << "\n";
            Utils::printDivider();
            int i = 1;
            for (auto& oi : aiList) {
                std::cout << "  " << std::left
                          << std::setw(5)  << i++
                          << std::setw(22) << oi.itemName
                          << std::setw(10) << oi.currentQty
                          << std::setw(12) << oi.orderedQty
                          << std::setw(6)  << (oi.aiSuggested ? "✓" : "✗") << "\n";
            }
            std::cout << "\n  Continue editing? [1] Done  [2] Edit  [3] Add  [4] Remove  [5] Cancel\n";
        }
    }

    if (aiList.empty()) {
        std::cout << "  No items to order.\n"; Utils::pause(); return;
    }

    // Get delivery date
    std::string deliveryDate = Utils::getInput("\n  Enter required delivery date (YYYY-MM-DD): ");

    // Final confirmation
    std::cout << "\n  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              FINAL ORDER CONFIRMATION                ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    double total = 0;
    for (auto& oi : aiList) {
        double lineTotal = oi.orderedQty * oi.unitPrice;
        total += lineTotal;
        std::cout << "  " << std::left << std::setw(22) << oi.itemName
                  << " x" << std::setw(5) << oi.orderedQty
                  << "  ₹" << std::fixed << std::setprecision(2) << lineTotal << "\n";
    }
    std::cout << "  ──────────────────────────────────────────────────────\n";
    std::cout << "  Delivery Date : " << deliveryDate << "\n";
    std::cout << "  Total Value   : ₹" << std::fixed << std::setprecision(2) << total << "\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n\n";

    std::string confirm = Utils::getInput("  Confirm and send to vendor? (yes/no): ");
    if (confirm != "yes") { std::cout << "  Order cancelled.\n"; Utils::pause(); return; }

    VendorOrder order = orderMgr.createOrder(aiList, deliveryDate, auth.getUsername());
    orderMgr.printOrderSummary(order);

    std::cout << "  📧 Triggering vendor notifications...\n";
    std::cout << "  Running: python3 ../python/alert_system.py "
              << order.orderId << " \"" << deliveryDate << "\"\n\n";

    std::string cmd = "cd ../python && python3 alert_system.py "
                    + std::to_string(order.orderId)
                    + " " + deliveryDate
                    + " 2>/dev/null || echo '  (Python alerts ready — run separately)'";
    system(cmd.c_str());

    Utils::pause();
}

// ─── ORDERS MENU ─────────────────────────────────────────────────────────────
void ordersMenu() {
    while (true) {
        Utils::clearScreen();
        Utils::printHeader("Vendor Orders");
        std::cout << "  [1] View All Orders\n";
        std::cout << "  [2] View Pending Orders\n";
        if (auth.hasPermission(Role::MANAGER))
            std::cout << "  [3] Mark Order as Delivered\n";
        std::cout << "  [0] Back\n\n";

        int choice = Utils::getIntInput("Choose option: ");
        if (choice == 0) break;

        else if (choice == 1) {
            Utils::clearScreen();
            Utils::printHeader("All Vendor Orders");
            auto& orders = orderMgr.getAll();
            if (orders.empty()) { std::cout << "  No orders yet.\n"; }
            else {
                std::cout << "  " << std::left
                          << std::setw(8)  << "Order#"
                          << std::setw(14) << "Created"
                          << std::setw(14) << "DeliveryBy"
                          << std::setw(12) << "ApprovedBy"
                          << std::setw(12) << "Status"
                          << std::setw(12) << "Value" << "\n";
                Utils::printDivider();
                for (auto& o : orders) {
                    std::cout << "  " << std::left
                              << std::setw(8)  << o.orderId
                              << std::setw(14) << o.createdDate
                              << std::setw(14) << o.deliveryDate
                              << std::setw(12) << o.approvedBy
                              << std::setw(12) << o.status
                              << "₹" << std::fixed << std::setprecision(2) << o.totalValue << "\n";
                }
            }
            Utils::pause();
        }

        else if (choice == 2) {
            auto pending = orderMgr.getPending();
            if (pending.empty()) std::cout << "  No pending orders.\n";
            else for (auto& o : pending) orderMgr.printOrderSummary(o);
            Utils::pause();
        }

        else if (choice == 3 && auth.hasPermission(Role::MANAGER)) {
            int id = Utils::getIntInput("Order ID to mark delivered: ");
            if (orderMgr.markDelivered(id)) {
                std::cout << "  ✅ Order #" << id << " marked as delivered.\n";
                std::cout << "  📦 Please update stock quantities in Inventory → Update Quantity.\n";
            } else {
                std::cout << "  ❌ Order not found.\n";
            }
            Utils::pause();
        }
    }
}

// ─── REPORTS MENU ────────────────────────────────────────────────────────────
void reportsMenu() {
    Utils::clearScreen();
    Utils::printHeader("Reports & AI Dashboard");

    std::cout << "  [1] 📊 Launch Full AI Dashboard (Python)\n";
    std::cout << "  [2] 📈 Demand Forecast Report\n";
    std::cout << "  [3] 🗓️  Expiry Prediction Report\n";
    std::cout << "  [4] 🏪 Inventory Summary\n";
    std::cout << "  [0] Back\n\n";

    int choice = Utils::getIntInput("Choose option: ");

    if (choice == 1) {
        std::cout << "\n  🤖 Launching SmartShelf AI Dashboard...\n";
        system("cd ../python && python3 dashboard.py 2>/dev/null || echo '  Run: python3 python/dashboard.py'");
        Utils::pause();
    }
    else if (choice == 2) {
        std::cout << "\n  📈 Running demand forecast...\n";
        system("cd ../python && python3 demand_forecast.py 2>/dev/null || echo '  Run: python3 python/demand_forecast.py'");
        Utils::pause();
    }
    else if (choice == 3) {
        std::cout << "\n  🗓️  Running expiry prediction...\n";
        system("cd ../python && python3 expiry_predictor.py 2>/dev/null || echo '  Run: python3 python/expiry_predictor.py'");
        Utils::pause();
    }
    else if (choice == 4) {
        Utils::clearScreen();
        Utils::printHeader("Inventory Summary");
        auto cats = inventory.getCategoryCount();
        std::cout << "  Total Items     : " << inventory.getAll().size() << "\n";
        std::cout << "  Total Value     : ₹" << std::fixed << std::setprecision(2)
                  << inventory.getTotalInventoryValue() << "\n";
        std::cout << "  Low Stock Items : " << inventory.getLowStockItems().size() << "\n";
        std::cout << "  Critical Items  : " << inventory.getCriticalItems().size() << "\n";
        std::cout << "  Expiring (7d)   : " << inventory.getExpiringItems(7).size() << "\n\n";
        std::cout << "  By Category:\n";
        for (auto& c : cats)
            std::cout << "    " << std::left << std::setw(20) << c.first << c.second << " items\n";
        Utils::pause();
    }
}

// ─── MAIN ────────────────────────────────────────────────────────────────────
int main() {
    Utils::clearScreen();
    Utils::printHeader("SMARTSHELF v1.0 — AI Inventory Intelligence Engine");
    std::cout << "  Bengaluru, Karnataka | smartshelf.in\n\n";

    int attempts = 0;
    while (attempts < 3) {
        if (loginScreen()) {
            mainMenu();
            Utils::clearScreen();
            Utils::printHeader("SMARTSHELF — Logged Out");
            std::cout << "  Goodbye, " << "come back soon!\n\n";
            attempts = 0;
            std::string again = Utils::getInput("Login again? (yes/no): ");
            if (again != "yes") break;
        } else {
            attempts++;
            if (attempts >= 3)
                std::cout << "  ❌ Too many failed attempts. Exiting.\n";
        }
    }
    return 0;
}
