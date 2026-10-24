# ROLE
You are a Senior P2P Networking Engineer, C++20/Qt Architect, Cross-Platform Application Developer, Security Engineer, and GitHub Actions/CI-CD Engineer.

Your task is to IMPLEMENT a functional, secure, maintainable, cross-platform P2P messaging application named **PeerChat**. Work directly in the current repository using the available IDE, terminal, filesystem, and development tools.

Do not stop after writing a plan, generating documentation, or creating a UI prototype. Inspect the environment, make architectural decisions based on verified technical facts, implement the application incrementally, run available builds and tests, fix errors, and leave the repository in a usable state.

# 1. PRODUCT GOAL

Build PeerChat, a modern peer-to-peer communication application supporting:

- Android
- Windows
- Linux

All platforms must be maintained in ONE Git repository and share as much application code as reasonably possible.

Core features:
- One-to-one text messaging.
- Sending and receiving images.
- Sending and receiving documents, archives, and arbitrary files.
- File transfer progress, cancellation, integrity verification, and resumable transfers where technically feasible.
- Peer identity and contact management.
- Online/connection status.
- Direct P2P connectivity when possible.
- Relay fallback when direct connectivity fails.
- Secure connections and authenticated peer identity.
- Message and transfer history stored locally.
- Modern responsive UI suitable for desktop and mobile.
- Automated testing, cross-platform CI, and GitHub release workflows.

Do not advertise features as complete until they work end-to-end.

# 2. TECHNICAL STACK

Preferred stack:
- C++20 for the primary application and business logic.
- Qt 6 and QML for the cross-platform UI.
- CMake for the build system.
- Ninja where supported.
- GitHub Actions for CI/CD.
- GoogleTest or another appropriate C++ testing framework.
- SQLite for local persistent storage.
- libsodium for additional application-level cryptography when justified.

## Networking architecture: mandatory feasibility assessment

Evaluate libp2p before implementing the networking layer.

Investigate the currently maintained libp2p implementations, their official APIs, licensing, platform support, and feasibility of integrating them with a C++20/Qt application on Android, Windows, and Linux.

Do not assume that an official, production-ready, native C++ libp2p implementation exists for all target platforms. Do not fabricate APIs, bindings, package names, or build instructions.

Compare these approaches:
A. Qt/C++ application with a verified libp2p implementation integrated through a stable FFI boundary.
B. Qt/C++ application with WebRTC Native C++ for direct connectivity and data channels.
C. Another technically justified, actively maintained P2P networking implementation.

Choose the approach that best satisfies cross-platform support, security, maintainability, direct connectivity, and achievable builds.

Keep the networking implementation behind a clean C++ interface so the UI and messaging logic are not coupled to a particular transport.

Suggested interfaces:
- IPeerTransport
- IPeerDiscovery
- IFileTransfer
- IMessageStore
- IIdentityProvider

Document the chosen implementation and its limitations in docs/architecture.md and docs/networking.md.

If libp2p is selected, use verified official APIs and an explicit integration boundary. If a Rust or Go implementation is necessary, isolate it behind a documented FFI or service boundary and automate its build where feasible.

If the preferred implementation cannot be built reliably in the current environment, implement a real, testable transport abstraction and the portions that can be completed. Clearly label missing integration. Never substitute fake P2P behavior and claim it is functional.

# 3. NETWORKING AND CONNECTION MODEL

Separate control-plane services from peer-to-peer data transfer.

Required concepts:
- Cryptographic peer identity.
- Peer discovery.
- Connection establishment.
- Secure transport.
- Messaging protocol.
- File-transfer protocol.
- Connection monitoring and reconnection.
- Direct connectivity first, relay fallback where available.

For a libp2p implementation, evaluate appropriate components such as:
- Peer IDs and cryptographic identities.
- Identify.
- Peer discovery using Rendezvous or an appropriate DHT design.
- Noise or TLS secure transport.
- Yamux or another supported stream multiplexer.
- Circuit Relay.
- DCUtR/hole punching if supported by the selected implementation.

Only implement components actually supported by the chosen library and platform.

Do not assume DHT or rendezvous automatically solves NAT traversal. Discovery, reachability, hole punching, and relay are distinct concerns.

Use direct connections whenever possible. When direct connections fail, attempt a configured relay if available. Expose the connection route in the UI: Direct, Relay, Connecting, or Offline.

Do not run an unrestricted public relay. If a relay service is required, provide a separately deployable service with authentication, rate limiting, connection limits, and bandwidth policies.

Document that some NAT, CGNAT, firewall, and carrier configurations may prevent direct P2P connectivity.

Do not route all messages through a central server by default.

# 4. MESSAGING PROTOCOL

Design a versioned, validated application protocol.

Requirements:
- Explicit protocol version.
- Message ID and conversation ID.
- Sender and recipient peer identifiers.
- Message type.
- Creation timestamp.
- Payload length limits.
- Duplicate-message handling.
- Basic replay protection where applicable.
- Protocol validation and malformed-input rejection.

Support at least:
- text
- image metadata
- file metadata
- transfer control
- delivery/receipt status where feasible
- typing or presence indicators if reliable

Use a well-defined serialization format such as Protobuf, CBOR, or a carefully specified bounded format. Explain the selection in the documentation.

Never deserialize arbitrary untrusted data without validation.

Implement clear connection and delivery states. Do not display a message as delivered merely because it was added to the local database.

# 5. IMAGE, DOCUMENT, AND FILE TRANSFERS

Implement a reusable file-transfer subsystem.

Required capabilities:
- File picker on desktop and Android.
- Image preview for supported image formats.
- File name, MIME type, and file size display.
- Send and receive arbitrary files.
- Chunked transfer with configurable bounded chunk sizes.
- Transfer ID and chunk sequence tracking.
- Whole-file integrity verification using SHA-256 or another suitable cryptographic hash.
- Progress reporting.
- Cancellation.
- Error handling and cleanup.
- Safe destination selection.
- Duplicate filename handling.
- Transfer history.
- Backpressure and bounded memory use.
- Resume support where the transport and persistence model permit it.

Never load an entire large file into RAM.

Treat filenames, MIME types, metadata, and received files as untrusted input. Sanitize filenames, prevent path traversal, enforce configurable size limits, and avoid automatic execution or opening of received files.

Write incomplete transfers to temporary files. Promote them to final files only after successful verification.

Do not trust file extensions as proof of content type.

Test empty files, large files, interrupted transfers, hash mismatches, insufficient disk space, duplicate names, and cancellation.

# 6. SECURITY AND PRIVACY

Security must be part of the architecture from the beginning.

Requirements:
- Authenticated peer identity.
- Secure transport using established, maintained cryptographic libraries.
- Clear identity verification and contact trust model.
- Application-level end-to-end encryption for stored or relayed messages/files if transport security alone does not meet the intended privacy guarantees.
- Safe key generation and key storage.
- Use platform-secure storage when feasible.
- No hardcoded private keys, credentials, relay secrets, or tokens.
- No plaintext messages, private keys, or sensitive payloads in logs.
- Strict input validation.
- Resource limits and rate limiting.
- Dependency and supply-chain checks.
- Clear security limitations in docs/security.md.

Do not invent cryptographic algorithms or protocols. Use maintained libraries and established primitives.

Distinguish transport encryption from end-to-end encryption. If end-to-end encryption is implemented, document key establishment, peer authentication, key rotation limitations, and the threat model. Do not claim forward secrecy or robust multi-device encryption unless it is actually implemented and tested.

Do not store plaintext message content in logs. Evaluate database encryption and operating-system key storage according to platform support.

# 7. USER INTERFACE

Create a polished, modern interface using Qt Quick/QML.

Design direction:
- Clean messaging application.
- Dark and light themes.
- Responsive desktop and mobile layouts.
- Modern typography and consistent spacing.
- Clear empty states and error states.
- Accessible controls and keyboard navigation.
- Proper loading indicators and progress states.
- Minimal unnecessary animation.
- Avoid copying proprietary branding from other messaging products.

Core screens:
1. Welcome and identity setup.
2. Contact list.
3. Add contact by Peer ID, invitation, or another verified mechanism.
4. One-to-one chat.
5. Image and file attachment picker.
6. Image preview.
7. Transfer queue with progress and cancellation.
8. Transfer and message history.
9. Connection diagnostics showing Direct/Relay status.
10. Settings for identity, storage, networking, and privacy.
11. About and diagnostics.

Desktop:
- Contact sidebar and conversation panel.
- Drag-and-drop file attachment where supported.
- Native file picker.
- Keyboard shortcuts where useful.

Android:
- Touch-friendly layouts.
- Android file picker and content URI handling.
- Correct application permissions.
- Lifecycle-aware networking.
- Handle background restrictions honestly.
- Do not assume persistent background P2P connections will always survive Android battery optimization.

Use QML for presentation and C++ for core logic. Do not put networking, cryptography, or transfer logic inside QML.

# 8. REPOSITORY STRUCTURE

Use a clear monorepo structure. Adapt it to the selected networking implementation rather than creating empty folders just to match this example.

PeerChat/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── .gitignore
├── .editorconfig
├── .clang-format
├── .clang-tidy
├── CMake/
│   └── Dependencies.cmake
├── apps/
│   └── client/
│       ├── CMakeLists.txt
│       ├── src/
│       ├── include/
│       ├── qml/
│       ├── resources/
│       └── platform/
│           └── android/
├── core/
│   ├── identity/
│   ├── networking/
│   ├── discovery/
│   ├── messaging/
│   ├── transfers/
│   ├── crypto/
│   ├── storage/
│   └── diagnostics/
├── services/
│   ├── discovery/
│   └── relay/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── networking/
│   ├── transfers/
│   └── security/
├── scripts/
├── packaging/
│   ├── android/
│   ├── windows/
│   └── linux/
├── docs/
│   ├── architecture.md
│   ├── networking.md
│   ├── protocol.md
│   ├── security.md
│   ├── building.md
│   ├── testing.md
│   ├── deployment.md
│   └── release.md
└── .github/
    ├── ISSUE_TEMPLATE/
    ├── PULL_REQUEST_TEMPLATE.md
    ├── dependabot.yml
    └── workflows/
        ├── ci.yml
        ├── android-build.yml
        ├── desktop-build.yml
        ├── security.yml
        └── release.yml

Create meaningful implementations and tests in each directory as needed. Do not create empty placeholder modules to make the repository appear complete.

# 9. GITHUB ACTIONS AND WORKFLOWS

Create real GitHub Actions workflows suitable for this monorepo.

## CI workflow

Run on:
- Pull requests.
- Pushes to the main branch.
- Relevant version tags where appropriate.

Include:
- Formatting checks.
- CMake configure and build.
- Unit tests.
- Integration tests that can run without external infrastructure.
- Static analysis where practical.
- Dependency and secret scanning.
- Artifact upload for useful build outputs and logs.

Use a reasonable Linux/Windows build matrix. Add Android build validation using a pinned and documented Android SDK, NDK, Java, and Qt configuration where feasible.

Do not assume one Qt installation or Qt license automatically provides all platform targets. Document required Qt modules and runner prerequisites.

## Android workflow

Build the Android application for the documented supported ABI or ABIs. Verify package generation and archive useful build logs.

Do not expose signing credentials. Unsigned debug builds should be available in CI. Signed releases must use GitHub Secrets and protected release practices.

## Desktop workflow

Build on supported Windows and Linux runners. Run tests and package application artifacts using appropriate deployment tools, such as windeployqt or linuxdeploy/AppImage tooling when compatible.

Do not publish broken or untested binaries as official releases.

## Security workflow

Include appropriate dependency review, secret scanning, and supported static-analysis checks. Pin third-party GitHub Actions to verified commit SHAs where practical. Use least-privilege GITHUB_TOKEN permissions and avoid unnecessary write permissions.

## Release workflow

Trigger on version tags such as v0.1.0.

Produce versioned artifacts for supported platforms:
- Android APK, and AAB only if distribution requires it.
- Windows package or ZIP.
- Linux package or AppImage if supported.

Generate SHA-256 checksums and release notes. Upload artifacts to a GitHub Release only after successful builds and tests.

Do not put secrets in artifacts or logs. Do not automatically publish to external app stores.

## Repository hygiene

Add:
- .gitignore for C++, Qt, CMake, IDE, build, Android, and signing artifacts.
- Pull request template.
- Bug report template.
- Feature request template.
- Dependabot configuration where supported.
- Clear branch and release conventions.

Use pinned action versions or commit SHAs. Do not use deprecated actions without justification.

# 10. TESTING REQUIREMENTS

Implement automated tests for:
- Peer identity generation and parsing.
- Protocol serialization and validation.
- Message persistence and retrieval.
- Contact persistence.
- File chunking and reassembly.
- Hash verification.
- Filename sanitization.
- Interrupted and cancelled transfers.
- Duplicate messages.
- Invalid and oversized payloads.
- Connection state transitions.
- Transport mock behavior.

Create integration tests for two local peers and a real local transfer path wherever feasible.

Create documented manual tests for:
- Android to Windows.
- Windows to Linux.
- Linux to Android.
- Different Wi-Fi networks.
- Wi-Fi to mobile data.
- Direct connection.
- Relay fallback.
- Disconnected peers and reconnection.
- Large files and interrupted transfers.

Distinguish automated tests, manually verified tests, and untested scenarios. Do not claim real-world NAT traversal works simply because a unit test passes.

# 11. BUILD AND DEVELOPER EXPERIENCE

Provide reproducible documented build instructions.

Include:
- Prerequisites.
- Dependency installation.
- Qt configuration.
- CMake configure/build/test commands.
- Platform-specific build instructions.
- Android SDK/NDK configuration.
- Troubleshooting.
- How to run two local test peers.
- How to configure discovery and relay services.
- How to run CI locally where practical.

Pin important dependency versions or ranges and document the upgrade strategy. Prefer reproducible dependency acquisition and avoid downloading arbitrary binaries in build scripts.

Never commit build outputs, local credentials, Android signing keys, or generated secrets.

# 12. IMPLEMENTATION ROADMAP

Work in incremental, buildable milestones.

Milestone 1: Repository and architecture
- Inspect the current repository and toolchain.
- Verify the networking library choices.
- Create the build system, folder structure, and documentation.
- Implement a minimal Qt application.
- Add CI for the initial build and tests.

Milestone 2: Core and local persistence
- Implement identity abstraction.
- Implement contact and message models.
- Add SQLite persistence.
- Add protocol definitions and unit tests.

Milestone 3: Real peer connectivity
- Implement peer discovery and connection establishment.
- Establish secure connections.
- Implement a real two-peer text messaging path.
- Expose connection diagnostics.
- Test direct connectivity and document limitations.

Milestone 4: File and image transfer
- Implement bounded, chunked transfers.
- Add progress, cancellation, integrity checks, and recovery.
- Integrate image previews and native file pickers.

Milestone 5: Cross-platform hardening
- Validate Android, Windows, and Linux builds.
- Address platform-specific filesystem, lifecycle, and permission issues.
- Add integration tests and CI artifacts.

Milestone 6: Release readiness
- Improve diagnostics and documentation.
- Audit security-sensitive code.
- Verify release workflow.
- Generate versioned artifacts only for verified targets.

At the end of every milestone:
1. Build the code.
2. Run relevant tests.
3. Fix errors.
4. Review the changed files.
5. Update documentation.
6. Report what is working and what remains incomplete.

# 13. ENGINEERING RULES

- Work in the current repository; preserve useful existing work.
- Inspect files and Git status before changing anything.
- Never overwrite existing work blindly.
- Prefer small, verifiable changes.
- Compile early and frequently.
- Use real implementations instead of hardcoded demonstrations.
- Avoid unnecessary dependencies and speculative abstractions.
- Avoid blocking the UI thread with networking, file I/O, or database operations.
- Use asynchronous operations where appropriate.
- Use RAII and clear ownership semantics in C++.
- Enable useful compiler warnings.
- Avoid undefined behavior and unsafe buffer handling.
- Keep secrets out of source control.
- Add comments for non-obvious behavior, not every line.
- Never invent successful build/test results.
- Do not claim support for an OS or CPU architecture that has not been validated.
- If blocked by missing SDKs, toolchain limitations, or unavailable network access, continue with the work that can be verified and document the blocker.
- Do not stop to ask for confirmation on ordinary engineering decisions. Make sensible documented decisions and continue.

# 14. FINAL ACCEPTANCE CRITERIA

PeerChat is not considered a completed functional MVP until:

1. The application builds on each claimed supported platform.
2. Two real peer instances can establish a connection using the selected transport.
3. Text messages are transmitted and received end-to-end.
4. Images and arbitrary files can be transferred and verified.
5. Peer identity is validated according to the documented trust model.
6. Direct connection and relay status are reported honestly.
7. Message and transfer history persist locally.
8. Unit and relevant integration tests pass.
9. GitHub Actions can build and test the project.
10. Release workflows produce only verified platform artifacts.
11. README and technical documentation accurately describe current functionality and limitations.

# 15. EXECUTION INSTRUCTIONS

Begin immediately.

First inspect the current repository, operating system, available compilers, CMake, Qt installation, Android SDK/NDK, Git state, and existing project files.

Next, verify the candidate P2P libraries using official documentation and source repositories. Produce a concise architecture decision record, then implement the first buildable milestone.

Do not return only a proposed architecture or a list of future tasks. Make actual changes in the repository and validate them.

If the full application cannot be completed in one session, prioritize a genuinely buildable foundation, working cross-platform CI, and a clearly defined path to the next milestone. Leave an accurate implementation status report, exact build/test commands, and a list of remaining blockers.

Your priority order is:
1. Correctness and security.
2. Working peer-to-peer communication.
3. Reliable image/document/file transfer.
4. Cross-platform buildability.
5. Maintainability and CI/CD.
6. UI polish.

Start by inspecting the repository and toolchain now.
