#include <crow.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

using namespace std::chrono_literals;

typedef websocketpp::client<websocketpp::config::asio> client;
typedef websocketpp::connection_hdl connection_hdl;

class RingBuffer {
public:
    RingBuffer(size_t capacity = 1000)
        : size(capacity), index(0), values(capacity, ""), timestamps(capacity, ""), read_index(0) {}

    // Fügt einen neuen Wert und Zeitstempel in den RingBuffer ein
    void push(const std::string& value, const std::string& timestamp) {
        std::lock_guard<std::mutex> lock(mutex);  // Thread-Sicherheit

        values[index] = value;
        timestamps[index] = timestamp;

        // Erhöhe den Index und stelle sicher, dass er im Bereich der Ringgröße bleibt
        index = (index + 1) % size;
    }

    // Gibt die neuesten Werte und Zeitstempel zurück und löscht sie
    std::vector<std::pair<std::string, std::string>> get_and_clear() {
        std::lock_guard<std::mutex> lock(mutex);  // Thread-Sicherheit

        std::vector<std::pair<std::string, std::string>> result;

        // Wenn der Leseindex kleiner als der aktuelle Index ist, gibt es neue Daten zum Senden
        while (read_index != index) {
            result.push_back({ values[read_index], timestamps[read_index] });
            values[read_index] = "";  // Lösche die Daten
            timestamps[read_index] = "";  // Lösche den Zeitstempel
            read_index = (read_index + 1) % size;  // Erhöhe den Leseindex
        }

        return result;
    }

private:
    size_t size;  // Kapazität des RingBuffers
    size_t index;  // Aktueller Index für die nächste Einfügung
    size_t read_index;  // Leseindex für die zu sendenden Daten
    std::vector<std::string> values;  // Die Werte im RingBuffer
    std::vector<std::string> timestamps;  // Die Zeitstempel
    std::mutex mutex;  // Mutex für Thread-Sicherheit
};

class DataCollector {
public:
    DataCollector() : ring_buffer(1000) {}

    void on_message(connection_hdl hdl, client::message_ptr msg) {
        std::string received_data = msg->get_payload();

        // Generiere aktuellen Zeitstempel
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string timestamp = std::ctime(&time);
        timestamp.pop_back();  // Entferne den Zeilenumbruch

        // Empfange die Daten und speichere sie im RingBuffer
        ring_buffer.push(received_data, timestamp);  
    }

    void collect_data_from_server(const std::string& url) {
        std::thread([this, url]() {
            client c;

            try {
                c.init_asio();
                c.set_message_handler(websocketpp::lib::bind(
                    &DataCollector::on_message, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
                websocketpp::lib::error_code ec;
                client::connection_ptr con = c.get_connection(url, ec);

                if (ec) {
                    std::cerr << "Error connecting: " << ec.message() << std::endl;
                    return;
                }

                c.connect(con);
                c.run();
            } catch (websocketpp::exception const& e) {
                std::cerr << "WebSocket exception: " << e.what() << std::endl;
            }
        }).detach();
    }

    RingBuffer& get_ring_buffer() {
        return ring_buffer;
    }

private:
    RingBuffer ring_buffer;
};

class WebSocketServer {
public:
    WebSocketServer() {
        // Initialisiere den DataCollector, der Daten von anderen Servern sammelt
        data_collector = std::make_shared<DataCollector>();

        // Sammle Daten von Server 1 und 2 in separaten Threads
        data_collector->collect_data_from_server("ws://localhost:8765/ws");
        data_collector->collect_data_from_server("ws://localhost:8766/ws");
    }

    void start() {
        crow::SimpleApp app;

        // WebSocket-Route für Clients
        CROW_WEBSOCKET_ROUTE(app, "/ws")
            .onopen([this](crow::websocket::connection& conn) {
                std::cout << "Client connected: " << &conn << std::endl;
                connections.push_back(&conn);
            })
            .onmessage([this](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
                std::cout << "Message from client: " << message << std::endl;
            })
            .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
                std::cout << "Client disconnected: " << &conn << " Reason: " << reason << std::endl;
                // Entferne den Client aus der Liste der Verbindungen
                connections.erase(std::remove(connections.begin(), connections.end(), &conn), connections.end());
            });

        // Starte die Daten-Sende-Schleife für Clients
        std::thread([this]() {
            while (true) {
                std::this_thread::sleep_for(10ms);  // Sende alle 10ms Daten
                send_data_to_clients();
            }
        }).detach();

        // Starte die Crow-App
        app.port(8080).run();
    }

private:
    void send_data_to_clients() {
        // Hole die zu sendenden Daten und lösche sie nach dem Abrufen
        auto messages = data_collector->get_ring_buffer().get_and_clear();

        // Sende die gesammelten Daten an alle verbundenen Clients
        for (auto& conn : connections) {
            for (auto& message : messages) {
                try {
                    // Sende die empfangenen Daten ohne Änderung weiter
                    conn->send_text(message.first);
                    std::cout << "Sent data to client: " << message.first << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error sending data to client: " << e.what() << std::endl;
                }
            }
        }
    }

    std::vector<crow::websocket::connection*> connections;
    std::shared_ptr<DataCollector> data_collector;
};

int main() {
    WebSocketServer server;
    server.start();
    return 0;
}

