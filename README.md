# Disaster Relief and Resource Management System (C++ / Crow / SQLite)

A modern, full-stack Disaster Relief and Resource Management System built with a **C++ Crow micro-framework** backend, a **persistent SQLite3 database**, and a **responsive web dashboard**.

Developed for a second-year engineering student's Object-Oriented Programming (OOP) curriculum, this project demonstrates clean C++ architecture, object modeling, separation of concerns, transactional integrity, and REST API communication.

---

## 1. Project Overview & Features

- **Dashboard**: Real-time counter metrics (Total Disasters, Available Resources, Pending Relief Requests, Relief Distributed), low-stock warning banners, recent incident tables, and visual inventory breakdown bars.
- **Disaster Management**: Record, view, edit, search, filter, and delete disaster incidents (Floods, Cyclones, Earthquakes, Landslides, Wildfires) with severity ratings (*Critical*, *High*, *Medium*, *Low*).
- **Resource Inventory**: Add and manage emergency relief supplies (Food Packets, Purified Water, First Aid Kits, Thermal Blankets, Rescue Equipment). Features strict validation to prevent negative stock and invalid inputs.
- **Relief Requests**: Community and field hospital assistance request queue with urgency ratings and dynamic status transitions (*Pending* &rarr; *Approved* &rarr; *In Progress* &rarr; *Completed*).
- **Resource Distribution Engine**: Transactional resource allocation that validates available stock, automatically deducts inventory, records an immutable distribution log, and updates associated field requests. Insufficient stock allocations are automatically rejected with clear diagnostic feedback.
- **Audit Reports**: Statistical incident summary and inventory report with dedicated print styling (`@media print`) for official relief documentation.
- **Sample Data Management**: Automatic database initialization on startup, with a one-click *"Re-seed Sample Data"* feature to reset and test realistic disaster scenarios.

---

## 2. Technology Stack

- **Backend**: C++ (C++17) using the [Crow C++](https://crowcpp.org/) web micro-framework.
- **Networking**: Standalone Asio headers + Windows Sockets (`ws2_32`, `mswsock`).
- **Database**: SQLite3 (embedded C engine) storing data persistently in `data/disaster_relief.db`.
- **Frontend**: HTML5, Modern CSS3 (responsive grid, variables, print stylesheets), and Vanilla ES6 JavaScript (Fetch API).
- **Design Theme**: Dark Navy / Charcoal sidebar (`#0f172a`), Emergency Red highlights (`#dc2626`), and clean white/light-gray content cards.

---

## 3. Object-Oriented Programming (OOP) Concepts Demonstrated

| OOP Concept | Implementation in Code |
| :--- | :--- |
| **Encapsulation** | All entity fields (`m_name`, `m_quantity`, `m_severity`, etc.) are declared `private` in [`Disaster`](include/disaster.h), [`Resource`](include/resource.h), [`ReliefRequest`](include/relief_request.h), and [`Distribution`](include/distribution.h). Access and modifications occur through public getters and validated setters. |
| **Inheritance** | [`BaseEntity`](include/base_entity.h) acts as an abstract base class with common fields (`m_id`, `m_createdAt`). `Disaster`, `Resource`, `ReliefRequest`, and `Distribution` inherit publicly from `BaseEntity`. |
| **Polymorphism & Abstract Classes** | `BaseEntity` defines the pure virtual function `virtual crow::json::wvalue toCrowJson() const = 0;`. Derived classes provide their own specialized implementations for polymorphic JSON serialization. |
| **Separation of Concerns** | Clear architectural boundaries between domain models (`include/`), the database abstraction layer ([`Database`](include/database.h)), service logic ([`DisasterReliefSystem`](include/system.h)), and the HTTP routing controller ([`main.cpp`](src/main.cpp)). |
| **RAII (Resource Acquisition Is Initialization)** | The [`Database`](include/database.h) class opens the SQLite connection handle in its constructor and safely finalizes statements and closes connections in its destructor. Parameterized statements use RAII locking with `std::lock_guard<std::mutex>` for thread safety. |
| **Standard Template Library (STL)** | Extensive use of `std::vector`, `std::unique_ptr`, `std::shared_ptr`, `std::string`, `std::map`, and STL algorithms for in-memory collections and aggregations. |

---

## 4. Project Directory Structure

```text
disaster-management/
├── include/                     # C++ Header Files
│   ├── asio.hpp                 # Standalone Asio networking header
│   ├── base_entity.h            # Abstract base class (Polymorphism & Inheritance)
│   ├── crow_all.h               # Crow web framework single header
│   ├── database.h               # Database class definition & SQLite CRUD
│   ├── disaster.h               # Disaster domain entity
│   ├── distribution.h           # Distribution consignment entity
│   ├── relief_request.h         # Relief request entity
│   ├── resource.h               # Resource inventory entity
│   ├── sqlite3.h                # SQLite3 C API header
│   └── system.h                 # DisasterReliefSystem service facade
├── src/                         # C++ Source Files
│   ├── database.cpp             # Database implementation with prepared statements
│   ├── disaster.cpp             # Disaster entity methods
│   ├── distribution.cpp         # Distribution entity methods
│   ├── main.cpp                 # Crow server routes & REST API controller
│   ├── relief_request.cpp       # Relief request methods
│   ├── resource.cpp             # Inventory methods & stock safety rules
│   ├── sqlite3.c                # SQLite3 C engine source
│   └── system.cpp               # System service logic & stock validation
├── frontend/                    # Single-Page Web Frontend
│   ├── index.html               # Semantic HTML5 dashboard layout & modals
│   ├── style.css                # Disaster management stylesheet & print rules
│   └── script.js                # Frontend REST client & DOM controller
├── data/                        # Persistent Data Directory
│   └── disaster_relief.db       # Persistent SQLite3 database file
├── build.bat                    # Windows one-click compilation script
├── Makefile                     # GNU Make build script
├── CMakeLists.txt               # CMake configuration
└── README.md                    # Project documentation & presentation guide
```

---

## 5. Prerequisites & Toolchain Setup

The project uses GCC 16+ (MinGW-w64) for Windows. A portable build toolchain is provided under `C:\Users\naren\tools\w64devkit\bin` and configured in the user PATH.

To verify your C++ compiler:
```powershell
g++ --version
gcc --version
```

---

## 6. How to Build and Run the Project

### Option A: Using the Windows Batch Script (Recommended for Beginners)
Double-click `build.bat` in Windows Explorer or run from PowerShell:
```powershell
.\build.bat
```

### Option B: Using GNU Make
```powershell
make
```

### Option C: Manual Command-Line Compilation
```powershell
# 1. Compile SQLite3 C engine:
gcc -O2 -Iinclude -c src/sqlite3.c -o build/sqlite3.o

# 2. Compile C++ sources and link:
g++ -std=c++17 -O2 -Iinclude -D_WIN32_WINNT=0x0601 `
    src/main.cpp `
    src/database.cpp `
    src/disaster.cpp `
    src/resource.cpp `
    src/relief_request.cpp `
    src/distribution.cpp `
    src/system.cpp `
    build/sqlite3.o `
    -lws2_32 -lmswsock -lpthread `
    -o DisasterReliefSystem.exe
```

### Running the Application
```powershell
.\DisasterReliefSystem.exe 8080
```
Then open your web browser and navigate to:
**[http://localhost:8080](http://localhost:8080)**

---

## 7. REST API Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/` | Serves the Single-Page Application (`frontend/index.html`) |
| `GET` | `/api/stats` | Returns aggregate dashboard metrics & recent activities |
| `GET` | `/api/reports` | Returns detailed audit data & category breakdowns |
| `POST`| `/api/reset-sample-data` | Re-seeds SQLite database with realistic initial data |
| `GET` | `/api/disasters` | Returns all recorded disaster incidents |
| `POST`| `/api/disasters` | Creates a new disaster incident |
| `PUT` | `/api/disasters/<id>` | Updates an existing disaster record |
| `DELETE`| `/api/disasters/<id>` | Deletes a disaster record |
| `GET` | `/api/resources` | Returns all inventory items |
| `POST`| `/api/resources` | Adds a new resource (validates positive stock) |
| `PUT` | `/api/resources/<id>` | Updates an inventory item |
| `DELETE`| `/api/resources/<id>` | Deletes an inventory item |
| `GET` | `/api/requests` | Returns all community relief requests |
| `POST`| `/api/requests` | Submits a new relief request |
| `PUT` | `/api/requests/<id>/status` | Updates request status (*Pending/Approved/In Progress/Completed*) |
| `DELETE`| `/api/requests/<id>` | Deletes a relief request |
| `GET` | `/api/distributions` | Returns all logged relief distributions |
| `POST`| `/api/distributions` | Dispatches aid, validates stock, & deducts inventory atomically |

---

## 8. Teacher Demonstration Walkthrough Guide

Use this step-by-step sequence when demonstrating the project to your professor or examiner:

1. **Architecture & Startup**:
   - Start the executable from the terminal: `.\DisasterReliefSystem.exe 8080`.
   - Highlight the console output showing that the **C++ Crow server** is listening on port 8080 and connected to SQLite (`data/disaster_relief.db`).
   - Open `http://localhost:8080` in the browser to show the loaded dashboard.

2. **Dashboard Overview**:
   - Point out the 4 summary cards: Total Disasters, Available Resources, Pending Relief Requests, and Relief Distributed.
   - Show the dynamic stock distribution chart and recent incident tables.

3. **Demonstrating Encapsulation & Input Validation**:
   - Navigate to **Resource Inventory** &rarr; click **Add New Resource**.
   - Attempt to add an item with a negative quantity (e.g. `-10`).
   - Show that the system rejects the input with an error message, preserving business rules.
   - Enter valid details (e.g., *"Emergency Trauma Kits"*, Category: *Medical*, Qty: `150`, Location: *Central Warehouse*).
   - Show the new item appearing in the table with an **"Adequate"** stock badge.

4. **Demonstrating Stock Deduction & Insufficient Stock Handling**:
   - Navigate to **Distribution**.
   - Select a resource with limited stock (e.g., Inflatable Rescue Boats with 18 units).
   - Enter `50` in the quantity input.
   - Show that the UI immediately displays a real-time warning: *"Requested quantity exceeds available stock!"* and disables the dispatch button.
   - If submitted directly via API, show that the C++ backend rejects it with HTTP 400 and an informative error message.
   - Now enter a valid quantity (e.g. `5` units), choose the disaster target, and submit.
   - Show the success confirmation, the new distribution logged in the history table, and navigate back to **Inventory** to show the boat count decreased from 18 to 13.

5. **Demonstrating Request Lifecycle**:
   - Navigate to **Relief Requests**.
   - Show a pending request &rarr; click **Approve**.
   - Click **Dispatch Aid** directly from the request row &rarr; verify the form pre-fills with the requested resource and destination.
   - Authorize the allocation &rarr; return to Requests and demonstrate that the status transitioned automatically to **Completed**.

6. **Demonstrating Persistence**:
   - Stop the backend server in the console (`Ctrl+C`).
   - Notice the frontend immediately displays the red **Offline Warning Banner**, demonstrating network resilience.
   - Restart `.\DisasterReliefSystem.exe 8080`.
   - Refresh the page and show that all added disasters, modified resources, and distribution history remain completely preserved in SQLite.

7. **Demonstrating Print-Friendly Reports**:
   - Navigate to **Reports** &rarr; click **Print Report / Save PDF**.
   - Show the clean audit sheet layout formatted specifically for official print distribution.

---

## 9. Troubleshooting & FAQ

### Q: Why compile `sqlite3.c` with `gcc` instead of `g++`?
**A:** `sqlite3.c` is written in standard C99. When compiled as C++ with `g++`, keywords like `new` inside internal structures and implicit `void*` casts produce compilation errors. Compiling `sqlite3.c` with `gcc -c` produces a clean C object file (`build/sqlite3.o`) that links seamlessly with the C++ application.

### Q: What if port 8080 is already in use?
**A:** You can launch the server on any other available port by passing the port number as a command-line argument:
```powershell
.\DisasterReliefSystem.exe 8081
```

### Q: How do I completely reset the database to clean sample data?
**A:** Click the **"Re-seed Sample Data"** button in the bottom of the sidebar, or send a POST request to `/api/reset-sample-data`.
