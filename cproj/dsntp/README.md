# DSNTP — Clean C Engineering Tree

| Item | Value |
|------|-------|
| Root | `cproj/dsntp/` |
| Baseline | **DSNTP-DRS-002 V2.0** (sibling merged DRS under `cproj/`) |
| Language | C11 |
| Build | CMake ≥ 3.16 |

This tree is the **primary development root**. The older Chinese-path skeleton under `基于AI的时间同步系统设计与实现/开发工程/` is read-only reference only.

## Layout

```text
cproj/dsntp/
├── include/dsntp/     # stable public headers
├── lib/src/           # static library dsntp_core
├── apps/agent|time-shim|ctl
├── tests/             # smoke
├── api/               # IF-CTL contracts (proto + OpenAPI)
├── deploy/            # systemd + poc-n5 topology
├── docs/01..05        # system / module / component / interface / fill order
└── scripts/gen_keys.sh
```

## Build

### CMake（推荐，Linux / MSYS2 / MSVC）

```bash
cmake -S cproj/dsntp -B cproj/dsntp/build
cmake --build cproj/dsntp/build
ctest --test-dir cproj/dsntp/build --output-on-failure
```

Binaries: `dsntp-agent`, `dsntp-time-shim`, `dsntp-ctl`, `test_smoke`.

Optional OpenSSL (M2): `-DDSNTP_WITH_OPENSSL=ON`.

### Makefile（GCC）

```bash
make -C cproj/dsntp test
```

### Windows TinyCC 回退（本机无 CMake 时）

```powershell
powershell -File cproj/dsntp/scripts/build_tcc.ps1
```

TCC 构建使用 `lib/src/net/udp_stub.c`；生产请用 CMake 链接真实 `udp.c`。

## Run (scaffold)

```bash
./build/dsntp-ctl -p 8080
./build/dsntp-agent -c apps/agent/agent.example.conf
./build/dsntp-time-shim --agent-sock /tmp/time-agent.sock
```

Agent main loop currently advances FSM with **TODO** hooks for Collecting/Consensus (M1 fill-in). See [docs/05-开发填空顺序.md](docs/05-开发填空顺序.md).

## Red lines (DRS §12 / FR-ARCH)

1. Magic=`0x5453594E` ("TSYN"); Version=`0x02`; Signature=**64B**; FAULT_NOTIFY=**0x06**
2. Control Server / NTP / wide sync must **not** rewrite consensus \(T_c\)
3. Samples &lt; 2f+1 → **do not** update \(T_c\)
4. PeerFault must **not** move local FSM into Fault
5. Reporter must **not** block inside UDP hot path
6. Gateway may feed chrony from synthetic time only; no external NTP

## Docs map

| Doc | Layer |
|-----|-------|
| [01-系统架构.md](docs/01-系统架构.md) | System |
| [02-模块划分.md](docs/02-模块划分.md) | Module |
| [03-组件设计.md](docs/03-组件设计.md) | Component |
| [04-接口契约.md](docs/04-接口契约.md) | Interface |
| [05-开发填空顺序.md](docs/05-开发填空顺序.md) | M1–M6 roadmap |
