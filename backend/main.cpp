
#include <iostream>
#include <string>
#include <sqlite3.h>
#include <limits>
using namespace std;

// Base class
class Person {
protected:
    string name;
    string email;

public:
    Person(string n, string e) {
        name = n;
        email = e;
    }

    virtual void display() {
        cout << "Name: " << name << endl;
        cout << "Email: " << email << endl;
    }

    virtual ~Person() {}
};

// Derived class
class Student : public Person {
private:
    string phone;
    string gender;
    string course;
    string semester;

public:
    Student(string n, string e, string p,
            string g, string c, string s)
        : Person(n, e) {
        phone = p;
        gender = g;
        course = c;
        semester = s;
    }

    void display() override {
        cout << "\nStudent Details\n";
        Person::display();
        cout << "Phone: " << phone << endl;
        cout << "Gender: " << gender << endl;
        cout << "Course: " << course << endl;
        cout << "Semester: " << semester << endl;
    }

    string getName() { return name; }
    string getEmail() { return email; }
    string getPhone() { return phone; }
    string getGender() { return gender; }
    string getCourse() { return course; }
    string getSemester() { return semester; }
};

// Database class
class Database {
private:
    sqlite3* db;

public:
    Database() {
        db = nullptr;

        if (sqlite3_open("../database/students.db", &db) != SQLITE_OK) {
            cout << "Database connection failed!" << endl;
            return;
        }

        string sql =
            "CREATE TABLE IF NOT EXISTS students ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "email TEXT NOT NULL,"
            "phone TEXT,"
            "gender TEXT,"
            "course TEXT,"
            "semester TEXT);";

        char* error = nullptr;

        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
            cout << "Table creation failed: " << error << endl;
            sqlite3_free(error);
        } else {
            cout << "Database connected successfully!" << endl;
        }
    }

    void addStudent(Student& student) {
        string sql =
            "INSERT INTO students (name,email,phone,gender,course,semester) "
            "VALUES (?,?,?,?,?,?);";

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cout << "Unable to prepare database query!" << endl;
            return;
        }

        sqlite3_bind_text(stmt, 1, student.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, student.getEmail().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, student.getPhone().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, student.getGender().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, student.getCourse().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, student.getSemester().c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cout << "Student registered successfully!" << endl;
        } else {
            cout << "Registration failed!" << endl;
        }

        sqlite3_finalize(stmt);
    }

    void displayStudents() {
        string sql = "SELECT * FROM students;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cout << "Unable to read student records!" << endl;
            return;
        }

        cout << "\n----- Registered Students -----\n";

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cout << "\nStudent ID: " << sqlite3_column_int(stmt, 0);
            cout << "\nName: " << sqlite3_column_text(stmt, 1);
            cout << "\nEmail: " << sqlite3_column_text(stmt, 2);
            cout << "\nCourse: " << sqlite3_column_text(stmt, 5);
            cout << "\nSemester: " << sqlite3_column_text(stmt, 6);
            cout << "\n-----------------------------\n";
        }

        sqlite3_finalize(stmt);
    }

    ~Database() {
        if (db != nullptr) {
            sqlite3_close(db);
        }
    }
};

int main() {
    Database database;

    int choice;

    do {
        cout << "\n===== STUDENT REGISTRATION SYSTEM =====\n";
        cout << "1. Register Student\n";
        cout << "2. Display All Students\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid choice!\n";
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 1) {
            string name, email, phone, gender, course, semester;

            cout << "Enter Full Name: ";
            getline(cin, name);

            cout << "Enter Email: ";
            getline(cin, email);

            cout << "Enter Phone: ";
            getline(cin, phone);

            cout << "Enter Gender: ";
            getline(cin, gender);

            cout << "Enter Course: ";
            getline(cin, course);

            cout << "Enter Semester: ";
            getline(cin, semester);

            Student student(name, email, phone, gender, course, semester);

            student.display();
            database.addStudent(student);
        }
        else if (choice == 2) {
            database.displayStudents();
        }
        else if (choice != 3) {
            cout << "Invalid choice!\n";
        }

    } while (choice != 3);

    cout << "Thank you for using the system!\n";

    return 0;
}
