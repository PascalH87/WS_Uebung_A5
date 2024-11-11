#include "include/json.hpp"
#include <crow.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cmath>

using json = nlohmann::json;
using namespace std::chrono_literals;

struct ClientSession {
    crow::websocket::connection* connection;
    std::atomic<double> frequency;
    std::atomic<double> value_min;
    std::atomic<double> value_max;
    std::atomic<bool> active;

    ClientSession(crow::websocket::connection* conn, double freq, double min_val, double max_val)
        : connection(conn), frequency(freq), value_min(min_val), value_max(max_val), active(true) {}
};

// Funktion für den gemeinsamen Sinuskurven-Thread (für alle Clients auf /wss)
void shared_sinus_message_sender(std::vector<crow::websocket::connection*>& connections, 
                                 std::atomic<bool>& active, 
                                 std::mutex& conn_mutex, 
                                 double& frequency, 
                                 double& value_min, 
                                 double& value_max) {
    double time = 0.0;

    while (active) {
        std::this_thread::sleep_for(7ms);

        // Sinuswert berechnen
        double amplitude = (value_max - value_min) / 2.0;
        double offset = (value_max + value_min) / 2.0;
        double value = amplitude * std::sin(frequency * time) + offset;
        time += 0.01;

        // Zeitstempel erstellen
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

        // JSON-Nachricht erstellen mit ID
        json message = {
            {"id", 2}, // ID immer auf 2 setzen
            {"timestamp", ss.str()},
            {"value", value}
        };

        // Nachricht senden
        std::string message_str = message.dump();
        std::lock_guard<std::mutex> lock(conn_mutex);
        for (auto conn_ptr : connections) {
            try {
                conn_ptr->send_text(message_str);
                std::cout << "Gesendete gemeinsame Nachricht: " << message_str << " an Client: " << conn_ptr << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Fehler beim Senden an Client " << conn_ptr << ": " << e.what() << std::endl;
                active = false;
            }
        }
    }

    std::cout << "shared_sinus_message_sender Thread beendet." << std::endl;
}

int main() {
    crow::SimpleApp app;

    // Einstellungen für Sinus
    double frequency = 1.0;
    double value_min = 0.0;
    double value_max = 20.0;
    std::vector<crow::websocket::connection*> connections;
    std::mutex conn_mutex;
    auto active = std::make_shared<std::atomic<bool>>(true);

    // Sammlung für individuelle Sessions
    std::unordered_map<crow::websocket::connection*, std::unique_ptr<ClientSession>> individual_sessions;
    std::mutex session_mutex;

    // Starten des shared_sinus_message_sender Threads für die /ws Route
    std::thread([&connections, active, &conn_mutex, &frequency, &value_min, &value_max]() {
        shared_sinus_message_sender(connections, *active, conn_mutex, frequency, value_min, value_max);
    }).detach();

    // /ws Route für Sinuskurven
    CROW_WEBSOCKET_ROUTE(app, "/ws")
    .onopen([&connections, &conn_mutex](crow::websocket::connection& conn) {
        std::cout << "Gemeinsame WebSocket-Verbindung geöffnet für Sinuskurve (Client): " << &conn << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.push_back(&conn);
    })
    .onmessage([&frequency, &value_min, &value_max](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
        std::cout << "Nachricht vom gemeinsamen Client (Sinuskurve) empfangen: " << message << std::endl;
        json jsonData;
        try {
            jsonData = json::parse(message);
            if (jsonData.contains("Frequency")) {
                frequency = jsonData["Frequency"];
                std::cout << "Empfangene gemeinsame Frequenz: " << frequency << std::endl;
            }
            if (jsonData.contains("Value_min") && jsonData.contains("Value_max")) {
                value_min = jsonData["Value_min"];
                value_max = jsonData["Value_max"];
                std::cout << "Empfangene gemeinsame Werte: Value_min = " << value_min << ", Value_max = " << value_max << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Parsen der Nachricht: " << e.what() << std::endl;
        }
    })
    .onclose([&connections, &conn_mutex](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "Gemeinsame WebSocket-Verbindung geschlossen für Client (Sinuskurve): " << &conn << " Grund: " << reason << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.erase(std::remove(connections.begin(), connections.end(), &conn), connections.end());
    });

    app.port(8766).multithreaded().run();
}

