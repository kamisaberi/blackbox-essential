# Blackbox: Autonomous Air-Gapped Cyber-Physical Security Platform

Blackbox is a high-performance, autonomous security platform designed for high-assurance, air-gapped environments, critical infrastructure, and defense networks. Built in native C++20 and powered by the `xinfer-essential` AI engine (`libxinfer.so`), Blackbox ingests network telemetry, system logs, and camera streams to detect and autonomously block threats in microseconds using Linux kernel eBPF/XDP packet filtering.

---

## Core Architecture and Principles

- Air-Gapped and Cloud-Independent: Operates 100% offline without external telemetry, cloud API dependencies, or internet connectivity.
- Microsecond Kernel Mitigation: Utilizes eBPF (Extended Berkeley Packet Filter) and XDP (eXpress Data Path) to drop malicious network traffic directly inside the Linux network driver before it reaches the operating system network stack.
- Powered by xInfer Essential: Embedded C++20 AI engine (`libxinfer.so`) provides ultra-fast inference on local NVIDIA GPUs, Intel NPUs, or Rockchip ARM NPUs.
- Cyber-Physical Threat Correlation: Correlates digital threats (such as port scans or SSH brute-force attempts) with physical events (such as camera perimeter breaches or unauthorized access) in real-time.
- Lock-Free High-Throughput Processing: Multi-threaded ingestion and execution architecture utilizing lock-free ring buffers to handle millions of events per second without dropping packets.
- Encrypted Local Audit Trail: Stores security events, threat metrics, and mitigation logs locally in an encrypted database for compliance and forensics.

---

## System Architecture Flow

```text
[ Network Telemetry (eBPF/PCAP) ]  \
[ System Logs (Auditd/Syslog)   ]  ---> [ Lock-Free Ring Buffer ] ---> [ xInfer Essential AI Engine ]
[ CCTV Cameras (RTSP/USB)       ]  /                                             |
                                                                                 v
[ eBPF Kernel IP Dropper ] <--- [ Autonomous Mitigation ] <--- [ Correlation & Scoring Engine ]
[ Process Killer         ]
[ Physical Alarm Relay   ]
```

---

## Repository File Structure

```text
blackbox/
├── CMakeLists.txt                    # Primary CMake build configuration
├── LICENSE                           # MIT License
├── README.md                         # Product documentation and user guide
├── install.sh                        # Automated system dependency installer
│
├── configs/                          # Default System Configurations
│   ├── blackbox.json                 # Core system settings and xInfer targets
│   └── rules.json                    # Threat correlation and response policies
│
├── include/
│   └── blackbox/                     # Public C++ Headers & Interfaces
│       ├── blackbox.hpp              # Master library header
│       ├── correlator.hpp            # Threat correlation engine header
│       ├── event.hpp                 # Unified SecurityEvent struct definition
│       ├── export.hpp                # Symbol visibility export macros
│       ├── mitigation.hpp            # Automated mitigation engine interface
│       ├── storage.hpp               # Lock-free buffer & audit storage header
│       └── types.hpp                 # EventType, ThreatLevel, and ActionType enums
│
├── src/
│   ├── main.cpp                      # Daemon entry point and event loop
│   │
│   ├── ai/                           # AI Threat Core (Powered by xinfer-essential)
│   │   ├── ai_manager.hpp            # Wrapper header for libxinfer.so integration
│   │   ├── ai_manager.cpp            # Model loading & stream queueing logic
│   │   ├── network_detector.cpp      # Network flow anomaly detection inference
│   │   └── vision_detector.cpp       # Perimeter intrusion & camera stream detection
│   │
│   ├── api/                          # Local Air-Gapped Web Server & WebSocket Engine
│   │   ├── http_server.hpp           # Embedded C++ HTTP REST server header
│   │   ├── http_server.cpp           # REST endpoints (/api/v1/threats, /api/v1/system)
│   │   ├── websocket.hpp             # Real-time WebSocket feed header
│   │   └── websocket.cpp             # Live threat feed streaming for web UI
│   │
│   ├── correlator/                   # Real-Time Threat Correlation Engine
│   │   ├── rules_engine.hpp          # Policy rule evaluator header
│   │   ├── rules_engine.cpp          # Multi-modal cyber-physical event correlator
│   │   ├── threat_scorer.hpp         # Threat severity scoring header
│   │   └── threat_scorer.cpp         # Real-time risk scoring calculation
│   │
│   ├── ingest/                       # Low-Latency Data Collectors
│   │   ├── log_ingest.hpp            # Syslog & Linux Auditd collector header
│   │   ├── log_ingest.cpp            # Asynchronous socket log reader
│   │   ├── network_ingest.hpp        # eBPF / XDP / PCAP packet sniffer header
│   │   ├── network_ingest.cpp        # Zero-copy packet capture & feature builder
│   │   ├── vision_ingest.hpp         # RTSP / USB camera collector header
│   │   └── vision_ingest.cpp         # Zero-copy camera frame ingestion pipeline
│   │
│   ├── mitigation/                   # Autonomous Response & Enforcement
│   │   ├── ebpf_blocker.hpp          # Sub-microsecond XDP firewall blocker header
│   │   ├── ebpf_blocker.cpp          # Kernel-level IP packet drop execution
│   │   ├── gpio_relay.hpp            # Physical siren & alarm relay header
│   │   ├── gpio_relay.cpp            # Hardware GPIO trigger implementation
│   │   ├── process_killer.hpp        # Process isolation & containment header
│   │   └── process_killer.cpp        # Automated malicious process termination
│   │
│   └── storage/                      # Audit Logging & In-Memory Queues
│       ├── database.hpp              # Encrypted audit database logger header
│       ├── database.cpp              # Local SQLite / RocksDB encrypted storage
│       ├── ring_buffer.hpp           # High-throughput lock-free ring buffer header
│       └── ring_buffer.cpp           # Lock-free event queue implementation
│
├── web/                              # Air-Gapped Web UI Frontend
│   ├── app.js                        # Live WebSocket telemetry visualizer
│   ├── index.html                    # Real-time threat monitor UI layout
│   └── style.css                     # Dark theme UI CSS
│
├── tests/                            # Unit Testing & Latency Benchmarks
│   ├── CMakeLists.txt                # Test build script
│   ├── benchmark_siem.cpp            # End-to-end microsecond latency benchmark
│   ├── test_ai_manager.cpp           # xinfer-essential integration unit test
│   ├── test_correlator.cpp           # Threat correlation rules unit test
│   ├── test_ingest.cpp               # Ingestion throughput & packet sniffer test
│   └── test_mitigation.cpp           # eBPF firewall packet blocking unit test
│
└── scripts/                          # System Services & Deployment Automation
    ├── blackbox.service              # Linux systemd service configuration file
    └── build_ebpf.sh                 # Script to compile kernel-level eBPF XDP C code
```

---

## System Requirements and Prerequisites

### Operating System and Kernel
- Linux Kernel 5.4 or higher (Kernel 5.15+ recommended for full eBPF XDP driver support)
- Operating System: Ubuntu 22.04 LTS, Debian 12, or RHEL 9

### Dependencies
- Compiler: GCC 10+ or Clang 12+ with C++20 support
- Build System: CMake 3.20 or higher
- System Libraries: `libelf-dev`, `zlib1g-dev`, `libbpf-dev`, `sqlite3`, `libsqlite3-dev`
- Embedded AI Engine: `xinfer-essential` shared library (`libxinfer.so`) installed in `/usr/local/lib`

---

## Installation and Build Guide

### 1. Run Dependency Setup Script

Execute the provided installation script to install required eBPF libraries, header files, and build tools:

```bash
chmod +x install.sh
sudo ./install.sh
```

### 2. Build xInfer Essential Engine Dependency

Blackbox requires `libxinfer.so` to execute AI models. Ensure `xinfer-essential` is compiled and installed:

```bash
cd /path/to/xinfer-essential
mkdir build && cd build
cmake .. -DXINFER_ENABLE_TENSORRT=ON -DXINFER_ENABLE_OPENVINO=ON
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 3. Build Blackbox

Compile the Blackbox core engine, eBPF kernels, and web API server:

```bash
cd /path/to/blackbox
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Configuration Guide

Blackbox behavior is controlled via JSON configuration files located in the `configs/` directory.

### Core Configuration (`configs/blackbox.json`)

```json
{
  "system": {
    "node_id": "blackbox-node-01",
    "log_level": "info",
    "http_port": 8443,
    "websocket_port": 8444
  },
  "ingestion": {
    "network_interface": "eth0",
    "enable_ebpf_xdp": true,
    "enable_syslog_listener": true,
    "syslog_port": 514,
    "rtsp_camera_url": "rtsp://192.168.1.100:554/stream1"
  },
  "ai_engine": {
    "xinfer_target": "TensorRT",
    "network_model_path": "models/network_threat_detector.engine",
    "vision_model_path": "models/perimeter_yolo.engine",
    "confidence_threshold": 0.85
  },
  "mitigation": {
    "auto_block_enabled": true,
    "ebpf_block_duration_seconds": 3600,
    "enable_gpio_alarm": false
  }
}
```

### Threat Policy Rules (`configs/rules.json`)

```json
{
  "rules": [
    {
      "rule_id": "RULE-101",
      "name": "Correlated Cyber-Physical Breach",
      "description": "Triggers when physical camera detects intruder near server rack during network port scan",
      "conditions": {
        "vision_event": "perimeter_intrusion",
        "network_event": "port_scan_detected",
        "time_window_seconds": 10
      },
      "action": {
        "severity": "CRITICAL",
        "mitigation": ["ebpf_block_ip", "trigger_gpio_alarm", "log_audit_db"]
      }
    }
  ]
}
```

---

## Running Blackbox

### Running in Foreground (Interactive Mode)

To run Blackbox directly in your terminal for testing or debugging:

```bash
sudo ./build/blackbox --config configs/blackbox.json
```

### Running as a System Daemon (systemd)

To install Blackbox as a system service that starts automatically on boot:

```bash
sudo cp scripts/blackbox.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now blackbox
```

Check system status and logs:

```bash
sudo systemctl status blackbox
sudo journalctl -u blackbox -f
```

---

## Air-Gapped Local Web Dashboard

Blackbox includes an embedded C++ HTTP and WebSocket server. You can access the real-time threat monitor from any browser connected to the local air-gapped network:

```text
URL: https://<BLACKBOX_IP_ADDRESS>:8443
```

Features available in the dashboard:
- Live event throughput meters (events/sec)
- Active threat alert stream
- eBPF blocked IP tables with manual unblock toggles
- System resource utilization (NPU/GPU/CPU memory meters)

---

## Latency and Performance Benchmarking

A standalone benchmark binary is included to evaluate end-to-end event processing latency (ingestion -> xInfer AI inference -> correlation -> eBPF kernel drop).

To run the benchmark:

```bash
sudo ./build/tests/benchmark_siem
```

### Benchmark Target Performance Summary

```text
==================================================
Blackbox End-to-End Threat Pipeline Benchmark
Engine Dependency : libxinfer.so (TensorRT Backend)
Test Iterations   : 100,000 Security Events
==================================================
Packet Ingestion Latency : 0.12 microseconds
xInfer AI Execution Time : 0.84 milliseconds
Correlation Evaluation   : 0.05 microseconds
eBPF Kernel IP Drop Time : 0.08 microseconds
--------------------------------------------------
Total Event-to-Mitigation: < 1.00 millisecond
==================================================
```

---

## License

Blackbox is released under the MIT License. See the `LICENSE` file for full terms and conditions.