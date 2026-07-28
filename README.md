# Agentic Policy Exploration

An experimental framework for exploring **LLM-driven workload scheduling policies** in distributed grid simulations.

Agentic Policy Exploration connects a Python-based AI scheduling agent to a native [CGSim](https://github.com/) dispatcher plugin. When a job arrives, the agent uses a tool call to determine which simulated grid site contains the largest share of the job’s required input data, then assigns the job to that site.

> **Project status:** Experimental research prototype.

## Overview

Traditional workload schedulers usually implement placement policies directly in application code. This project explores a different architecture: a language-model agent selects and invokes scheduling tools while the simulation remains responsible for calculating authoritative results and executing the resulting decision.

The current policy is intentionally constrained:

1. CGSim sends an incoming job to the Python agent.
2. The agent calls the `most_data_located` tool.
3. The C++ plugin determines which site contains the most required input data.
4. Sites without enough remaining storage for the job’s outputs are excluded.
5. The agent returns the selected site.
6. CGSim assigns an available CPU at that site and runs the job.

The provided simulation configuration models **30 sites**, named `Site0` through `Site29`.

## Architecture

```text
┌──────────────────────────────┐
│          CGSim Grid          │
│                              │
│  Workload → Dispatcher       │
│                 │            │
│                 ▼            │
│      C++ Dispatcher Plugin   │
└──────────────┬───────────────┘
               │ JSON over TCP
               │ 127.0.0.1:5000
               ▼
┌──────────────────────────────┐
│       Python AI Agent        │
│                              │
│  OpenRouter-compatible LLM   │
│             │                │
│             ▼                │
│   most_data_located tool     │
└──────────────┬───────────────┘
               │ tool request
               ▼
┌──────────────────────────────┐
│      C++ Policy Logic        │
│                              │
│ Data locality + free storage │
└──────────────────────────────┘
```

The language model does not calculate data locality itself. It is instructed to call the simulation-provided tool exactly once and use the returned site without modification.

## Repository Structure

```text
Agentic-Policy-Exploration/
├── agent/
│   ├── agent.py          # LLM client, prompt, tool definition, and scheduler
│   ├── main.py           # Main agent message loop
│   ├── mcp_tools.py      # Bridge for simulation-provided tools
│   └── server.py         # TCP JSON server
├── config/
│   ├── config.json              # Main CGSim configuration
│   ├── site_connections.json    # Inter-site bandwidth and latency
│   └── site_topology.json       # Compute and storage topology
├── plugin/
│   ├── CMakeModules/     # CMake package-discovery modules
│   ├── include/          # C++ headers
│   ├── src/
│   │   ├── AgenticPolicyExplorationPlugin.cpp
│   │   ├── client.cpp
│   │   ├── dispatcher.cpp
│   │   ├── output.cpp
│   │   └── workload_manager.cpp
│   └── CMakeLists.txt
└── LICENSE
```

## Requirements

### Python

* Python 3.10 or newer
* `openai` Python package
* An OpenRouter API key
* Network access to the configured model provider

### C++

* A compiler with C++17 support
* CMake 3.12 or newer
* Boost
* SQLite3
* SimGrid
* CGSim

The required native libraries must be installed where CMake can locate them.

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/REDWOOD24/Agentic-Policy-Exploration.git
cd Agentic-Policy-Exploration
```

### 2. Set up the Python agent

Create and activate a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

On Windows PowerShell:

```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
```

Install the Python dependency:

```bash
pip install openai
```

Set your OpenRouter API key:

```bash
export OPENROUTER_API_KEY="your-api-key"
```

On Windows PowerShell:

```powershell
$env:OPENROUTER_API_KEY="your-api-key"
```

### 3. Build the dispatcher plugin

```bash
cmake -S plugin -B plugin/build
cmake --build plugin/build
```

The plugin is built as a shared library named approximately:

```text
libAgenticPolicyExplorationPlugin.so
```

On macOS, the extension is normally `.dylib`. On Windows, it is normally `.dll`.

If CMake cannot locate a dependency, provide the appropriate installation prefix:

```bash
cmake \
  -S plugin \
  -B plugin/build \
  -DCMAKE_PREFIX_PATH="/path/to/dependencies"
```

## Configuration

The primary simulation configuration is stored in:

```text
config/config.json
```

The default configuration resembles:

```json
{
  "Grid_Name": "GRID",
  "Sites_Information": "site_topology.json",
  "Sites_Connection_Information": "site_connections.json",
  "Dispatcher_Plugin": "../plugin/build/libAgenticPolicyExplorationPlugin.dylib",
  "Limited_Sites": [],
  "Custom_Parameters": {
    "Num_of_Jobs": "200",
    "output_file": "../output/events.db"
  }
}
```

### Plugin path

Update `Dispatcher_Plugin` for your operating system and build output.

Linux example:

```json
"Dispatcher_Plugin": "../plugin/build/libAgenticPolicyExplorationPlugin.so"
```

macOS example:

```json
"Dispatcher_Plugin": "../plugin/build/libAgenticPolicyExplorationPlugin.dylib"
```

### Simulation parameters

| Parameter                      | Description                                 |
| ------------------------------ | ------------------------------------------- |
| `Grid_Name`                    | Name of the simulated grid                  |
| `Sites_Information`            | Path to the site topology definition        |
| `Sites_Connection_Information` | Path to network connection data             |
| `Dispatcher_Plugin`            | Path to the compiled shared library         |
| `Limited_Sites`                | Optional subset of permitted sites          |
| `Num_of_Jobs`                  | Number of jobs generated for the experiment |
| `output_file`                  | SQLite database used for simulation events  |

## Running an Experiment

The agent opens a TCP server on:

```text
127.0.0.1:5000
```

Start the Python agent first:

```bash
cd agent
python main.py
```

You should see:

```text
Agent waiting for communication...
```

In another terminal, start CGSim with `config/config.json` using the command provided by your CGSim installation.

For example:

```bash
<path-to-cgsim-executable> config/config.json
```

The precise CGSim executable name and invocation may depend on how CGSim was installed.

During execution, the terminals will display JSON communication between the simulation and the agent.

## Communication Protocol

Messages are newline-delimited JSON objects sent over TCP.

### Job assignment request

The plugin sends:

```json
{
  "request_type": "assign_job",
  "job_id": "example-job-id",
  "expect_reply": true
}
```

### Tool request

The Python agent requests the data-locality tool:

```json
{
  "request_type": "tool",
  "tool_type": "most_data_located"
}
```

### Tool response

The plugin responds with the selected site:

```json
{
  "response_type": "tool",
  "tool_result": "Site7",
  "expect_reply": true
}
```

### Scheduling decision

The agent sends the final result:

```json
{
  "site_decision": "Site7"
}
```

## Current Scheduling Policy

The `most_data_located` implementation:

* examines the locations of a job’s input files;
* counts how many required files are available at each site;
* ranks sites by that count;
* verifies that the candidate site has sufficient remaining storage for all output files;
* returns the highest-ranked valid site; and
* raises an error when no suitable site is available.

After selecting a site, the dispatcher searches for a host that:

* belongs to the selected site;
* is not a job server;
* is not a communication server; and
* has enough available CPU cores for the job.

## Model Configuration

The model is configured in `agent/agent.py`:

```python
model = "inclusionai/ling-3.0-flash:free"
```

To use another OpenRouter-compatible model, replace that value:

```python
model = "provider/model-name"
```

Choose a model that reliably supports function or tool calling and structured JSON output.

## Extending the Project

### Add a scheduling tool

A new tool generally requires changes in both layers:

1. Implement the authoritative scheduling calculation in the C++ plugin.
2. Add a corresponding message type in the plugin’s dispatcher loop.
3. Add a Python wrapper in `agent/mcp_tools.py`.
4. Register the tool in `agent/agent.py`.
5. Update the system prompt with explicit usage rules.

Potential tools include:

* available CPU capacity;
* estimated queue time;
* network transfer cost;
* remaining storage;
* energy consumption;
* data-transfer latency; and
* multi-objective site ranking.

### Explore new policies

Possible experiments include:

* data-locality-only placement;
* compute-capacity-aware scheduling;
* network-aware scheduling;
* storage-aware scheduling;
* energy-aware scheduling;
* weighted multi-objective placement; and
* comparisons between deterministic and agentic policies.

## Limitations

* The agent currently listens only on `127.0.0.1:5000`.
* Only one simulation connection is accepted at a time.
* The active agent policy exposes one tool.
* The valid site names are fixed to `Site0` through `Site29` in the prompt.
* The model name is hard-coded.
* Dependency versions are not pinned.
* Automated tests and continuous integration are not currently included.
* Malformed model output or socket messages may terminate an experiment.
* The current site-locality score counts file locations rather than summing total file sizes.
* The project should be treated as an experimental prototype rather than a production scheduler.

## Troubleshooting

### `OPENROUTER_API_KEY is not set`

Export the key before starting the agent:

```bash
export OPENROUTER_API_KEY="your-api-key"
```

### Agent remains at “waiting for communication”

Confirm that:

* the Python agent was started before CGSim;
* the simulation plugin was loaded successfully;
* both processes are running on the same machine; and
* port `5000` is available.

### Plugin library cannot be loaded

Check that `Dispatcher_Plugin` points to the correct file and uses the correct platform extension.

You can inspect the build directory with:

```bash
find plugin/build -name "*AgenticPolicyExplorationPlugin*"
```

### CMake cannot find CGSim or SimGrid

Provide the dependency prefix explicitly:

```bash
cmake \
  -S plugin \
  -B plugin/build \
  -DCMAKE_PREFIX_PATH="/path/to/cgsim;/path/to/simgrid"
```

### Model does not return valid JSON

Try a model with stronger tool-calling and structured-output support. You may also add validation, retries, and timeout handling around the API call before conducting larger experiments.

## Research Considerations

When evaluating agentic policies, consider recording:

* job completion time;
* queue waiting time;
* input transfer volume;
* input transfer duration;
* site utilization;
* scheduling latency;
* storage utilization;
* API response time;
* invalid scheduling decisions; and
* model or API cost.

For meaningful comparisons, run each policy across multiple random seeds and report both central tendency and variability.

## Security

Do not commit API keys or other credentials to the repository.

The current TCP interface has no authentication or encryption and is intended for local experimentation only. Avoid exposing port `5000` to untrusted networks.

## Contributing

Contributions are welcome through issues and pull requests.

Useful contribution areas include:

* additional scheduling tools;
* policy baselines;
* configuration validation;
* error handling and retry logic;
* automated tests;
* reproducible experiment scripts;
* containerized development environments; and
* analysis and visualization utilities.

## License

This project is licensed under the Apache License 2.0. See [`LICENSE`](LICENSE) for the complete terms.
