#include "include/json.hpp"
#include <crow.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_map>

using json = nlohmann::json;
using namespace std::chrono_literals;

struct ClientSession {
    crow::websocket::connection* connection;
    std::atomic<int> value;
    int value_min;
    int value_max;
    std::atomic<bool> active;

    ClientSession(crow::websocket::connection* conn, int min_val, int max_val)
        : connection(conn), value(min_val), value_min(min_val), value_max(max_val), active(true) {}
};

// Funktion zum Senden von Nachrichten an alle verbundenen Clients in der /ws Route
void message_sender(std::vector<crow::websocket::connection*>& connections, 
                    std::atomic<bool>& active, 
                    std::mutex& conn_mutex, 
                    int& value_min, 
                    int& value_max, 
                    int id) {
    int value = value_min;

    while (active) {
        std::this_thread::sleep_for(3ms); 

        if (value >= value_max) {
            value = value_min;
        } else {
            value++;
        }

        // Zeitstempel mit Millisekunden erstellen
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

        // JSON-Nachricht erstellen
        json message = {
            {"timestamp", ss.str()},
            {"value", value},
            {"id", id}
        };

        // Nachricht als String konvertieren
        std::string message_str = message.dump();

        // Senden der Nachricht an alle verbundenen Clients in /ws
        std::lock_guard<std::mutex> lock(conn_mutex);
        for (auto conn_ptr : connections) {
            try {
                conn_ptr->send_text(message_str);
                std::cout << "Gesendete Nachricht: " << message_str << " an Client: " << conn_ptr << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Fehler beim Senden an Client " << conn_ptr << ": " << e.what() << std::endl;
                active = false;
            }
        }
    }

    std::cout << "message_sender Thread beendet." << std::endl;
}

int main() {
    crow::SimpleApp app;

    int value_min = 0;
    int value_max = 20;
    std::vector<crow::websocket::connection*> connections;
    std::mutex conn_mutex;
    auto active = std::make_shared<std::atomic<bool>>(true);

    // Starten des message_sender Threads für die /ws Route
    std::thread([&connections, active, &conn_mutex, &value_min, &value_max]() {
        message_sender(connections, *active, conn_mutex, value_min, value_max, 1);  // ID 1 für die /ws Route
    }).detach();


    // Neue /wsn Route für alle Clients mit der ID 1
    CROW_WEBSOCKET_ROUTE(app, "/ws")
    .onopen([&connections, &conn_mutex](crow::websocket::connection& conn) {
        std::cout << "WebSocket-Verbindung geöffnet für Client in /ws: " << &conn << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.push_back(&conn);
    })
    .onmessage([&value_min, &value_max](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
        std::cout << "Nachricht vom Client empfangen in /wsn: " << message << std::endl;
        json jsonData;
        try {
            jsonData = json::parse(message);
            if (jsonData.contains("Value_min") && jsonData.contains("Value_max")) {
                value_min = jsonData["Value_min"];
                value_max = jsonData["Value_max"];
                std::cout << "Empfangene Werte: Value_min = " << value_min << ", Value_max = " << value_max << std::endl;
            } else {
                std::cerr << "Ungültige Nachricht: Keine Value_min oder Value_max gefunden" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Parsen der Nachricht: " << e.what() << std::endl;
        }
    })
    .onclose([&connections, &conn_mutex](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "WebSocket-Verbindung geschlossen für Client in /wsn: " << &conn << " Grund: " << reason << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.erase(std::remove(connections.begin(), connections.end(), &conn), connections.end());
    });

    app.port(8765).multithreaded().run();
    return 0;
}

