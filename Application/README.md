# CoDriver — Mobile & Web App Description

## Project Overview
CoDriver is a companion mobile and web application for the Codriver embedded system. It provides drivers and fleet managers with real-time telemetry, trip summaries, alerts, and configuration tools to improve safety, efficiency, and vehicle monitoring.

## Mobile App (Android / iOS)
Purpose:
- In-vehicle companion for drivers.
- Real-time HUD, trip recording, and driver alerts.

Key Features:
- Live telemetry (speed, location, sensor status).
- In-trip event detection and push notifications (hard braking, lane departure).
- Trip summary, fuel/energy consumption, and driver scorecards.
- Offline buffering when connectivity is lost; upload when restored.
- Simple settings: driver profile, alert thresholds, privacy controls.

UX/Behavior:
- Minimal driver distraction: large readable UI, voice alerts, configurable alert levels.
- Background operation with low-power location updates.
- Local caching encrypted on device.

Permissions & Privacy:
- Location, notifications, background execution (platform-specific).
- Explicit consent for telemetry sharing; option to anonymize or opt-out.

## Shared Backend & APIs
- REST and WebSocket APIs provide telemetry ingestion, command/control, and real-time updates.
- Authentication: OAuth2 / JWT for sessions and API access.
- Data retention policies and role-based access control.
- Webhooks and third-party integrations (e.g., mapping, alerts).

## Data Flow
1. Device/embedded system streams telemetry to backend.
2. Backend persists and processes events; pushes real-time updates via WebSocket.
3. Mobile app subscribes for live updates and uploads buffered data.
4. Web app queries APIs for aggregated analytics and historical data.

## Tech Stack (Suggested)
- Mobile: React Native 
- Backend: Node.js with PostgreSQL and Redis
- Real-time: WebSockets or MQTT
- CI/CD: GitHub Actions, automated builds and deploys

## Deployment & Local Setup (high level)
- Configure .env with API endpoints and credentials.
- Start backend services (DB, cache, API).
- Run mobile app: yarn && react-native run-android / run-ios (or open native projects)

## Security & Compliance
- TLS everywhere, input validation, rate limiting.
- Minimal data retention; pseudonymization where required.

## Contribution
- Follow branching, tests required for PRs.
- Update docs when adding APIs, config, or UI flows.

## Contact / Next Steps
- Document API contract (OpenAPI) and telemetry schema.
- Provide mobile UX flows and mockups for handoff.
- Setup CI pipelines for automated builds and OTA releases.
