#pragma once
#include <string>
#include <map>

enum class Role { MANAGER, CASHIER, VIEWER };

struct User {
    std::string username;
    std::string password;
    Role role;
};

class Auth {
private:
    std::map<std::string, User> users;
    User* currentUser = nullptr;

public:
    Auth() {
        users["manager"] = {"manager", "manager123", Role::MANAGER};
        users["cashier"] = {"cashier", "cashier123", Role::CASHIER};
        users["viewer"]  = {"viewer",  "viewer123",  Role::VIEWER};
    }

    bool login(const std::string& username, const std::string& password) {
        if (users.count(username) && users[username].password == password) {
            currentUser = &users[username];
            return true;
        }
        return false;
    }

    void logout() { currentUser = nullptr; }

    bool isLoggedIn() const { return currentUser != nullptr; }

    Role getRole() const { return currentUser->role; }

    std::string getUsername() const {
        return currentUser ? currentUser->username : "";
    }

    bool hasPermission(Role required) const {
        if (!currentUser) return false;
        return static_cast<int>(currentUser->role) <= static_cast<int>(required);
    }

    std::string roleToString() const {
        if (!currentUser) return "None";
        switch (currentUser->role) {
            case Role::MANAGER: return "Manager";
            case Role::CASHIER: return "Cashier";
            case Role::VIEWER:  return "Viewer";
        }
        return "Unknown";
    }
};
