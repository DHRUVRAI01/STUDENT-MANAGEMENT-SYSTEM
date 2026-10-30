
const form = document.getElementById("studentForm");
const message = document.getElementById("message");

form.addEventListener("submit", async function(event) {
    event.preventDefault();

    const student = {
        name: document.getElementById("name").value,
        email: document.getElementById("email").value,
        phone: document.getElementById("phone").value,
        gender: document.getElementById("gender").value,
        course: document.getElementById("course").value,
        semester: document.getElementById("semester").value
    };

    message.style.color = "blue";
    message.innerText = "Registering student...";

    try {
        const response = await fetch("http://127.0.0.1:8080/register", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(student)
        });

        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.error || "Registration failed");
        }

        message.style.color = "green";
        message.innerText =
            result.message + " Student ID: " + result.studentId;

        form.reset();

    } catch (error) {
        message.style.color = "red";
        message.innerText = "Error: " + error.message;
        console.error(error);
    }
});
async function loadStudents() {
    const list = document.getElementById("studentList");

    try {
        const response = await fetch("http://127.0.0.1:8080/students");
        const students = await response.json();

        if (students.length === 0) {
            list.innerHTML = "<p>No students registered yet.</p>";
            return;
        }

        let html = `
            <table border="1" cellpadding="10">
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Email</th>
                    <th>Course</th>
                    <th>Semester</th>
                </tr>
        `;

        students.forEach(student => {
            html += `
                <tr>
                    <td>${student.id}</td>
                    <td>${student.name}</td>
                    <td>${student.email}</td>
                    <td>${student.course}</td>
                    <td>${student.semester}</td>
                </tr>
            `;
        });

        html += "</table>";
        list.innerHTML = html;

    } catch (error) {
        list.innerText = "Unable to load student records.";
        console.error(error);
    }
}