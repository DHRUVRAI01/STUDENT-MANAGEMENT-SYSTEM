
#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using json = nlohmann::json;

const int PORT = 8080;
sqlite3* db = nullptr;

// Create database table
bool setupDatabase() {
    if (sqlite3_open("database/students.db", &db) != SQLITE_OK) {
        cout << "Database connection failed!\n";
        return false;
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS students ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "email TEXT NOT NULL,"
        "phone TEXT,"
        "gender TEXT,"
        "course TEXT,"
        "semester TEXT);";

    char* error = nullptr;

    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        cout << "Database error: " << error << endl;
        sqlite3_free(error);
        return false;
    }

    cout << "Database connected successfully!\n";
    return true;
}

// Send HTTP response
void sendResponse(SOCKET client, int status,
                  const string& body,
                  const string& contentType = "application/json") {
    string statusText = status == 200 ? "OK" :
                        status == 201 ? "Created" :
                        status == 400 ? "Bad Request" :
                        status == 404 ? "Not Found" : "Server Error";

    string response =
        "HTTP/1.1 " + to_string(status) + " " + statusText + "\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: " + to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n" + body;

    send(client, response.c_str(), static_cast<int>(response.size()), 0);
}

// Handle each website request
void handleRequest(SOCKET client) {
    string request;
    char buffer[4096];

    size_t headerEnd = string::npos;
    size_t contentLength = 0;

    while (true) {
        int received = recv(client, buffer, sizeof(buffer), 0);

        if (received <= 0) return;

        request.append(buffer, received);

        headerEnd = request.find("\r\n\r\n");

        if (headerEnd != string::npos) {
            string headers = request.substr(0, headerEnd);

            size_t pos = headers.find("Content-Length:");

            if (pos != string::npos) {
                contentLength = stoul(headers.substr(pos + 15));
            }

            if (request.size() >= headerEnd + 4 + contentLength) {
                break;
            }
        }
    }

    string firstLine = request.substr(0, request.find("\r\n"));
    istringstream line(firstLine);

    string method, path;
    line >> method >> path;

    if (method == "OPTIONS") {
        sendResponse(client, 200, "{}");
        return;
    }

    if (method == "POST" && path == "/register") {
        try {
            string body = request.substr(headerEnd + 4, contentLength);
            json data = json::parse(body);

            string name = data.at("name").get<string>();
            string email = data.at("email").get<string>();
            string phone = data.at("phone").get<string>();
            string gender = data.at("gender").get<string>();
            string course = data.at("course").get<string>();
            string semester = data.at("semester").get<string>();

            const char* sql =
                "INSERT INTO students "
                "(name,email,phone,gender,course,semester) "
                "VALUES (?,?,?,?,?,?);";

            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                sendResponse(client, 500, R"({"error":"Database query failed"})");
                return;
            }

            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, phone.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, gender.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, course.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, semester.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                int id = static_cast<int>(sqlite3_last_insert_rowid(db));
                sqlite3_finalize(stmt);

                sendResponse(client, 201,
                    json{{"success", true},
                         {"message", "Student registered successfully!"},
                         {"studentId", id}}.dump());
            } else {
                sqlite3_finalize(stmt);
                sendResponse(client, 500, R"({"error":"Unable to save student"})");
            }
        }
        catch (const exception& e) {
            sendResponse(client, 400,
                json{{"error", e.what()}}.dump());
        }

        return;
    }

    if (method == "GET" && path == "/students") {
        json students = json::array();

        const char* sql =
            "SELECT id,name,email,phone,gender,course,semester FROM students;";

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                students.push_back({
                    {"id", sqlite3_column_int(stmt, 0)},
                    {"name", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))},
                    {"email", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))},
                    {"phone", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))},
                    {"gender", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))},
                    {"course", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))},
                    {"semester", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))}
                });
            }
            sqlite3_finalize(stmt);
        }

        sendResponse(client, 200, students.dump());
        return;
    }

    sendResponse(client, 404, R"({"error":"Route not found"})");
}

int main() {
    if (!setupDatabase()) {
        return 1;
    }

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Winsock startup failed!\n";
        sqlite3_close(db);
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddress.sin_port = htons(PORT);

    if (serverSocket == INVALID_SOCKET ||
        bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR ||
        listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Server could not start. Check whether port 8080 is in use.\n";
        if (serverSocket != INVALID_SOCKET) closesocket(serverSocket);
        WSACleanup();
        sqlite3_close(db);
        return 1;
    }

    cout << "C++ server running at http://127.0.0.1:8080\n";
    cout << "Keep this terminal open while using the website.\n";

    while (true) {
        SOCKET client = accept(serverSocket, nullptr, nullptr);

        if (client == INVALID_SOCKET) continue;

        handleRequest(client);
        closesocket(client);
    }

    closesocket(serverSocket);
    WSACleanup();
    sqlite3_close(db);

    return 0;
}
