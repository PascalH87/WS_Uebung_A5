# BUILD.md - Anleitung zur Installation und zum Build-Prozess

Dieses Dokument beschreibt die Schritte, um das Projekt **WS_Uebung_A5: Multi Producer - Aggregator** zu bauen und auszuführen. Das Projekt besteht aus einem Python WebSocket-Client und mehreren C++ WebSocket-Servern.

## Voraussetzungen

### 1. **Python:**
- Version 3.6 oder höher.
- Folgende Python-Bibliotheken müssen installiert sein:
  - `websocket-client` für WebSocket-Kommunikation.
  - `matplotlib` für die Datenvisualisierung.

### 2. **C++:**
- Ein C++17-kompatibler Compiler (z.B. GCC, Clang oder MSVC).
- Die folgenden C++-Bibliotheken müssen installiert sein:
  - **Crow** für WebSocket- und HTTP-Server.
  - **WebSocketPP** für WebSocket-Client-Kommunikation.
  - **nlohmann/json** für JSON-Verarbeitung.

## **Installation**

### 1. **Python Client installieren:**

Führe die folgenden Schritte aus, um die Python-Abhängigkeiten zu installieren:

1. Stelle sicher, dass **Python 3.6 oder höher** installiert ist.
2. Installiere die erforderlichen Python-Bibliotheken:
   ```bash
   pip install websocket-client matplotlib

3. Stelle sicher, dass Tkinter für die GUI verfügbar ist (wird normalerweise mit Python installiert).

2. C++ Server installieren:

Crow Framework installieren:
Clone das Crow-Repository:
bash
git clone https://github.com/CrowCpp/Crow.git
Folge den Anweisungen im Crow GitHub Repository zur Installation und Einrichtung von Crow.

WebSocketPP installieren:
Clone das WebSocketPP-Repository:
bash
git clone https://github.com/zaphoyd/websocketpp.git
Folge den Anweisungen im WebSocketPP GitHub Repository zur Installation und Einrichtung von WebSocketPP.

nlohmann/json installieren:
Clone das nlohmann/json-Repository:
bash
git clone https://github.com/nlohmann/json.git
Folge den Anweisungen im nlohmann/json GitHub Repository zur Installation und Einrichtung von nlohmann/json.

Build der C++ Server
Die C++-Server (Server 1, Server 2 und Server 3) müssen mit einem C++-Compiler gebaut werden.

Server 1 und Server 2:
Erstelle einen neuen CMake-Projektordner und baue den Server:
bash
mkdir build
cd build
cmake ..
make server1
make server2

Server 3:
Server 3 verwendet zusätzliche Abhängigkeiten (WebSocketPP), daher wird der Build-Prozess entsprechend angepasst:
bash
mkdir build
cd build
cmake ..
make server3

3. Ausführung der C++ Server
Starte Server 1 und Server 2:

In einem Terminal, starte Server 1:
bash
./server1
In einem weiteren Terminal, starte Server 2:
bash
./server2

Starte Server 3:
In einem dritten Terminal, starte Server 3:
bash
./server3

Nutzung des Python Clients
Starte den Python Client:
Der Python Client kann durch Ausführen des folgenden Befehls gestartet werden:
bash
python client.py
Der Client öffnet ein Fenster, in dem du die WebSocket-URI eines der C++-Server eingeben kannst (z. B. ws://localhost:8080/ws für Server 3).

Verbindung zum WebSocket-Server herstellen:
Gib die URI eines der C++-Server ein (z. B. ws://localhost:8080/ws für Server 3) und klicke auf "Start".
Der Client zeigt dann die aktuellen Datenpunkte und einen Plot mit den empfangenen Werten an.

Interaktionen im Client:
Zeige die letzten 100 Datenpunkte an.
Zeige den neuesten Datenpunkt an.
Anpassung der angezeigten Kurven im Plot.

Lizenz
Dieses Projekt ist unter der MIT-Lizenz lizenziert. Weitere Informationen findest du in der LICENSE-Datei.

