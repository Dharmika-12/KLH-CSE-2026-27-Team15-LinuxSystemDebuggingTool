async function startDebugger() {
    const output = document.getElementById("debuggerOutput");
    const statusMessage = document.getElementById("statusMessage");
    const processStatus = document.getElementById("processStatus");
    const syscallCount = document.getElementById("syscallCount");
    const registerStatus = document.getElementById("registerStatus");
    const memoryStatus = document.getElementById("memoryStatus");
    const resourceStatus = document.getElementById("resourceStatus");

    // Get selected target program
    const selectedProgram =
        document.getElementById("programSelect").value;

    // Only allow the three supported choices
    const allowedPrograms = ["target_program", "ls", "whoami"];

    if (!allowedPrograms.includes(selectedProgram)) {
        statusMessage.textContent = "Invalid target program.";
        return;
    }

    // Update dashboard
    processStatus.textContent = "Running";
    registerStatus.textContent = "Reading";
    memoryStatus.textContent = "Reading";
    resourceStatus.textContent = "Running";
    output.textContent = "Starting debugger...";
    statusMessage.textContent =
        `Starting debugger for ${selectedProgram}...`;

    try {
        // Send selected program to the C server
        const url =
            `http://localhost:8080/run?program=${encodeURIComponent(selectedProgram)}`;

        const response = await fetch(url);

        if (!response.ok) {
            throw new Error("Server error: " + response.status);
        }

        const text = await response.text();

        // Display target process information
        const targetName =
            text.match(/\[Target Process\]\s*Name\s*:\s*(.*)/)?.[1]?.trim()
            || "Unknown";

        const targetProgram =
            text.match(/\[Target Process\]\s*Program\s*:\s*(.*)/)?.[1]?.trim()
            || "Unknown";

        document.getElementById("targetName").textContent = targetName;
        document.getElementById("targetProgram").textContent = targetProgram;

        // Count system calls
        const syscallMatches =
            text.match(/\[Syscall Entry\]/g);

        const syscallTotal = syscallMatches
            ? syscallMatches.length
            : 0;

        syscallCount.textContent = syscallTotal;

        // Check debugger modules
        const registersRead = text.includes("CPU REGISTERS");
        const memoryRead = text.includes("MEMORY READER");
        const resourcesRead = text.includes("RESOURCE MONITOR");

        registerStatus.textContent =
            registersRead ? "Read" : "Not Read";

        memoryStatus.textContent =
            memoryRead ? "Read" : "Not Read";

        resourceStatus.textContent =
            resourcesRead ? "Complete" : "Not Available";

        processStatus.textContent = "Completed";
        statusMessage.textContent =
            `Debugging completed for ${targetName}.`;

        // Get the first 10 system-call numbers
        const syscallNumbers = [];

        const matches = text.matchAll(
            /\[Syscall Entry\]\s*Number:\s*(\d+)/g
        );

        for (const match of matches) {
            if (syscallNumbers.length < 10) {
                syscallNumbers.push(match[1]);
            }
        }

        // Extract resource information
        const getValue = (pattern) =>
            text.match(pattern)?.[1]?.trim() || "N/A";

        const userTime = getValue(
            /\[Resource\]\s*CPU User Time\s*:\s*(.*)/
        );

        const systemTime = getValue(
            /\[Resource\]\s*CPU System Time\s*:\s*(.*)/
        );

        const maxMemory = getValue(
            /\[Resource\]\s*Maximum Memory\s*:\s*(.*)/
        );

        const totalRam = getValue(
            /\[Resource\]\s*Total RAM\s*:\s*(.*)/
        );

        const freeRam = getValue(
            /\[Resource\]\s*Free RAM\s*:\s*(.*)/
        );

        const availableRam = getValue(
            /\[Resource\]\s*Available RAM\s*:\s*(.*)/
        );

        // Display resource details
        document.getElementById("userTime").textContent = userTime;
        document.getElementById("systemTime").textContent = systemTime;
        document.getElementById("maxMemory").textContent = maxMemory;
        document.getElementById("totalRam").textContent = totalRam;
        document.getElementById("freeRam").textContent = freeRam;
        document.getElementById("availableRam").textContent = availableRam;

        // Display debugger summary
        output.textContent = `
=====================================
          DEBUGGER SUMMARY
=====================================

Target Process : ${targetName}
Program        : ${targetProgram}
Process Status : Completed
System Calls   : ${syscallTotal}
CPU Registers  : ${registersRead ? "Read" : "Not Read"}
Memory Reader  : ${memoryRead ? "Read" : "Not Read"}
Resources      : ${resourcesRead ? "Complete" : "Not Available"}

-------------------------------------
        SYSTEM RESOURCES
-------------------------------------

CPU User Time   : ${userTime}
CPU System Time : ${systemTime}
Maximum Memory  : ${maxMemory}
Total RAM       : ${totalRam}
Free RAM        : ${freeRam}
Available RAM   : ${availableRam}

-------------------------------------
        FIRST SYSTEM CALLS
-------------------------------------

${syscallNumbers.length > 0
    ? syscallNumbers.join(", ")
    : "No system calls detected"}

=====================================
       DEBUGGING COMPLETED
=====================================
`;
    } catch (error) {
        processStatus.textContent = "Error";
        syscallCount.textContent = "0";
        registerStatus.textContent = "Error";
        memoryStatus.textContent = "Error";
        resourceStatus.textContent = "Error";

        statusMessage.textContent = "Debugger failed.";

        output.textContent = `
=====================================
          DEBUGGER ERROR
=====================================

Could not connect to the C debugger server.

Error:
${error.message}

Check that the C server is running:
./server
`;
    }
}

async function loadDebugHistory() {
    const body = document.getElementById("historyBody");
    const status = document.getElementById("historyStatus");

    status.textContent = "Loading debugging history...";
    body.replaceChildren();

    try {
        const response = await fetch("http://localhost:8080/history");

        if (!response.ok) {
            throw new Error("Server error: " + response.status);
        }

        const history = await response.json();

        if (!Array.isArray(history) || history.length === 0) {
            const row = body.insertRow();
            const cell = row.insertCell();
            cell.colSpan = 3;
            cell.textContent = "No saved debugging sessions found.";
            status.textContent = "0 sessions found.";
            return;
        }

        for (const item of history) {
            const row = body.insertRow();

            const fileCell = row.insertCell();
            fileCell.textContent = item.file;

            const programCell = row.insertCell();
            programCell.textContent = item.program;

            const actionCell = row.insertCell();
            const button = document.createElement("button");

            button.textContent = "View Log";

            button.addEventListener("click", async () => {
                try {
                    const logUrl =
                        "http://localhost:8080/log?file=" +
                        encodeURIComponent(item.file);

                    const logResponse = await fetch(logUrl);

                    if (!logResponse.ok) {
                        throw new Error("Could not load log.");
                    }

                    const logText = await logResponse.text();

                    document.getElementById("debuggerOutput").textContent =
                        logText;

                    document.getElementById("statusMessage").textContent =
                        "Viewing saved log: " + item.file;

                    document.getElementById("debuggerOutput")
                        .scrollIntoView({ behavior: "smooth" });

                } catch (error) {
                    status.textContent =
                        "Could not load log: " + error.message;
                }
            });

            actionCell.appendChild(button);
        }

        status.textContent =
            history.length + " saved session(s) found.";

    } catch (error) {
        console.error("History loading failed:", error);

        const row = body.insertRow();
        const cell = row.insertCell();
        cell.colSpan = 3;
        cell.textContent = "Unable to load debugging history.";

        status.textContent =
            "Could not load history: " + error.message;
    }
}

async function loadThreads() {
    const pidInput = document.getElementById("threadPid");
    const status = document.getElementById("threadStatus");
    const output = document.getElementById("threadOutput");

    const pid = pidInput.value.trim();

    // Validate the process ID
    if (!/^[1-9]\d*$/.test(pid)) {
        status.textContent = "Please enter a valid positive process ID.";
        output.textContent = "No thread information available.";
        return;
    }

    status.textContent = "Loading thread information...";
    output.textContent = "Contacting the Linux debugger server...";

    try {
        const response = await fetch(
            `http://localhost:8080/threads?pid=${encodeURIComponent(pid)}`
        );

        if (!response.ok) {
            throw new Error("Server returned status " + response.status);
        }

        const result = await response.text();

        // Display the server response safely as text
        output.textContent = result;

        if (
            result.includes("Invalid process ID") ||
            result.includes("Process not found") ||
            result.includes("Please provide a process ID")
        ) {
            status.textContent = "Could not retrieve threads. Check the PID.";
        } else {
            const countMatch = result.match(/Total threads:\s*(\d+)/);

            if (countMatch) {
                status.textContent =
                    `Successfully retrieved ${countMatch[1]} thread(s) for PID ${pid}.`;
            } else {
                status.textContent = "Thread information received.";
            }
        }
    } catch (error) {
        status.textContent = "Unable to contact the debugger server.";
        output.textContent =
            "Error: " + error.message +
            "\n\nMake sure the C server is running on port 8080.";
    }
}

async function refreshProcessInfo() {
    const parentElement = document.getElementById("parentPid");
    const currentElement = document.getElementById("currentPid");
    const status = document.getElementById("threadStatus");

    try {
        // Ask the server for its own process information.
        const response = await fetch("http://localhost:8080/process-info");

        if (!response.ok) {
            throw new Error("Server returned status " + response.status);
        }

        const data = await response.json();

        parentElement.textContent = data.ppid ?? "--";
        currentElement.textContent = data.pid ?? "--";

        status.textContent = "Process IDs refreshed.";
    } catch (error) {
        status.textContent =
            "Could not load process IDs. The server endpoint must be added first.";
    }
}

function useCurrentPid() {
    const pid = document.getElementById("currentPid").textContent;

    if (!/^[1-9]\d*$/.test(pid)) {
        document.getElementById("threadStatus").textContent =
            "Refresh process IDs first.";
        return;
    }

    document.getElementById("threadPid").value = pid;
    loadThreads();
}


async function findThreadDemo() {
    const status = document.getElementById("threadStatus");
    const pidInput = document.getElementById("threadPid");
    const output = document.getElementById("threadOutput");

    status.textContent = "Starting or locating thread_demo...";
    output.textContent = "Please wait...";

    try {
        let foundPid = null;

        // First request starts the demo if it is not running.
        // Later requests locate the running process.
        for (let attempt = 0; attempt < 20; attempt++) {
            const response = await fetch(
                "http://localhost:8080/find-thread-demo",
                { cache: "no-store" }
            );

            if (!response.ok) {
                throw new Error("Server returned HTTP " + response.status);
            }

            const data = await response.json();

            if (data.error) {
                throw new Error(data.error);
            }

            if (data.pid) {
                foundPid = data.pid;
                break;
            }

            status.textContent =
                "Waiting for thread_demo to start...";

            await new Promise(resolve => setTimeout(resolve, 500));
        }

        if (!foundPid) {
            throw new Error(
                "Could not detect thread_demo. Check that the executable exists."
            );
        }

        pidInput.value = foundPid;

        status.textContent =
            "Found thread_demo (PID " + foundPid + "). Loading threads...";

        await loadThreads();

    } catch (error) {
        status.textContent = "Could not start or find thread_demo: " +
            error.message;

        output.textContent =
            "Check that the C server is running on port 8080 " +
            "and that thread_demo exists in the project root.";
    }
}

