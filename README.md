# WS_Uebung_A5
Multi Producer - Aggregator

### **Projektübersicht**
Dieses Projekt besteht aus einem Python WebSocket-Client und drei C++ WebSocket-Servern. Der Python-Client empfängt und visualisiert Daten von den C++-Servern. Jeder Server hat unterschiedliche Aufgaben:

- **Server 1** sendet kontinuierlich Ganzzahlwerte.
- **Server 2** sendet Sinuskurven-Daten.
- **Server 3** abonniert die Daten von Server 1 und Server 2, speichert sie in einem Ringpuffer und stellt einen aggregierten Stream an seine Clients bereit, der alle 10ms die letzten gültigen Werte beider Server weitergibt.

### **Architektur**

#### **Komponenten:**

- **Python Client (WebSocket-Client)**:
  - Stellt eine Verbindung zu den C++ WebSocket-Servern her.
  - Empfängt und visualisiert Daten von den Servern.
  - Zeigt Zeitstempel und Werte für die beiden Datenströme an.
  - Bietet Interaktionsmöglichkeiten (z. B. Anzeige der letzten 100 Datenpunkte).

- **Server 1 (C++ mit Crow)**:
  - Sendet fortlaufende Ganzzahlen an alle verbundenen Clients.
  - Die Werte bewegen sich innerhalb eines vordefinierten Bereichs, der über WebSocket-Nachrichten angepasst werden kann.

- **Server 2 (C++ mit Crow)**:
  - Sendet Sinuskurven-Werte an alle verbundenen Clients.
  - Die Frequenz und der Wertebereich der Sinuskurve können durch WebSocket-Nachrichten vom Client aus angepasst werden.

- **Server 3 (C++ mit Crow und WebSocketPP)**:
  - Dieser Server empfängt Daten von Server 1 und Server 2, speichert diese in einem Ringpuffer und sendet alle 10ms die letzten gültigen Werte beider Server an alle verbundenen Clients.

#### **Funktionsweise von Server 3:**
- **Abonnement der Daten von Server 1 und Server 2**:
  - Server 3 hört kontinuierlich auf WebSocket-Nachrichten von den beiden anderen Servern und speichert die empfangenen Daten (einschließlich Zeitstempel) in einem Ringpuffer.

- **Datenaggregation**:
  - Alle 10ms liest Server 3 die letzten gültigen Werte aus dem Ringpuffer und sendet diese als aggregierten Stream an alle verbundenen Clients.

- **Sample-and-Hold-Strategie**:
  - Wenn keine neuen Daten von Server 1 oder Server 2 eingetroffen sind, hält Server 3 den letzten gültigen Wert und sendet diesen an die Clients weiter.

### **Installation**

#### **Voraussetzungen:**
- **Python**: Version 3.6 oder höher.
- **C++**: Mit Unterstützung für Crow WebSocket, WebSocketPP und nlohmann/json.

#### **Abhängigkeiten:**

**Python:**
- websocket-client für WebSocket-Kommunikation.
- matplotlib für die Datenvisualisierung.

**C++:**
- Crow Framework für WebSocket und HTTP.
- WebSocketPP für WebSocket-Client-Funktionalität.
- nlohmann/json für JSON-Verarbeitung.

#### **Python Client-Installation:**
1. Installiere die benötigten Python-Bibliotheken:
   ```bash
   pip install websocket-client matplotlib
Stelle sicher, dass du tkinter für die GUI hast (wird standardmäßig mit Python installiert).

### **C++ Server-Installation:**
1. **Installiere Crow:** [Crow GitHub](https://github.com/CrowCpp/Crow)
2. **Installiere WebSocketPP:** [WebSocketPP GitHub](https://github.com/zaphoyd/websocketpp)
3. **Installiere nlohmann/json:** [JSON for Modern C++ GitHub](https://github.com/nlohmann/json)

### **Nutzung**

#### **Python Client:**

1. **Starte den Python Client:**
   - Der Client startet ein Fenster, in dem du die WebSocket-URI der Server eingeben kannst.
   - Der Client zeigt die aktuellen Datenpunkte und ermöglicht es, die letzten 100 oder den neuesten Datenpunkt anzuzeigen.
   - Zusätzlich wird ein Plot mit den Daten angezeigt.

2. **Verbindung zum WebSocket-Server:**
   - Gib die URI eines der C++-Server (z. B. ws://localhost:8080/ws für Server 3) ein und klicke auf "Start".
   - Sobald die Verbindung hergestellt ist, beginnt der Client, Daten vom Server zu empfangen und anzuzeigen.

3. **Interaktion mit dem Client:**
   - Zeige die letzten 100 Datenpunkte an.
   - Zeige den neuesten Datenpunkt an.
   - Anpassung der angezeigten Kurven im Plot.

#### **C++ Server 1:**
- **WebSocket-Route:** /ws
- **Funktion:** Der Server sendet kontinuierlich Ganzzahlen in einem vordefinierten Bereich.
- **Konfiguration:** Der Bereich kann über WebSocket-Nachrichten angepasst werden, die von einem Client gesendet werden.

#### **C++ Server 2:**
- **WebSocket-Route:** /ws
- **Funktion:** Der Server sendet Sinuskurven-Werte in einem vordefinierten Bereich.
- **Konfiguration:** Die Frequenz und der Wertebereich der Sinuskurve können über WebSocket-Nachrichten vom Client aus angepasst werden.

#### **C++ Server 3:**
- **WebSocket-Route:** /ws
- **Funktion:** Dieser Server empfängt Daten von Server 1 und Server 2, speichert diese in einem Ringpuffer und stellt einen aggregierten Stream bereit, der alle 10ms die letzten gültigen Werte beider Server an die Clients weitergibt.
- **Abonnement:** Server 3 abonniert die Daten von Server 1 und Server 2.
- **Datenaggregation:** Alle 10ms wird der letzte gültige Wert beider Server in einem aggregierten Stream an die verbundenen Clients gesendet.

### **Lizenz**
Dieses Projekt ist unter der MIT-Lizenz lizenziert. Siehe LICENSE für Details.

