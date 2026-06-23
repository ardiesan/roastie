# AI Roast Device

An ESP32 camera that detects a face, sends it to AWS Lambda, and plays back a short, witty, safe spoken roast through a speaker. Built for a live demo at an AWS Community Day.

## Architecture

```
[ xmini-c3 CAM ]                                   ┌─────────────────────────────┐
   | (1) local face detect (ESP-WHO)               │  AWS Lambda (Agentic Brain) │
   | (2) capture JPEG                               │  ─────────────────────────  │
   | (3) HTTPS POST image bytes ──────────────────► │  perceive → reason → act    │
   |                                                │   ├─ Rekognition DetectFaces│
   |                                                │   ├─ Bedrock (Claude) + GR  │
   |                                                │   ├─ safety gate / fallback │
   |                                                │   └─ Polly SynthesizeSpeech │
   | (4) ◄──────────────── audio bytes (WAV/PCM) ───┤  (S3 only as scratch/log)   │
   v                                                └─────────────────────────────┘
[ I2S DAC (MAX98357A) ] → speaker
```

## Quick Start

1. [Enable Bedrock model access](docs/BEDROCK-SETUP.md)
2. Deploy cloud infrastructure: `bash scripts/deploy.sh`
3. Configure firmware: `cp firmware/main/secrets.example.h firmware/main/secrets.h` and fill in values
4. Flash firmware: `bash scripts/flash.sh /dev/ttyUSB0`

## Repo Structure

| Directory | Contents |
|-----------|----------|
| [firmware/](firmware/) | ESP32 firmware (ESP-IDF), xmini-c3 pinout |
| [backend/](backend/) | Lambda handler, agent loop, safety module, unit tests |
| [infra/](infra/) | AWS SAM template — one-command deploy/teardown |
| [docs/](docs/) | Wiring/BOM, demo runbook, architecture, privacy, Bedrock setup |
| [scripts/](scripts/) | Deploy, teardown, warm-up, flash, fallback clip generation |

## Phase Docs

| Phase | Doc | Status |
|-------|-----|--------|
| 1 | [01-scaffold.md](01-scaffold.md) | Done |
| 2 | [02-backend.md](02-backend.md) | Pending |
| 3 | [03-infra.md](03-infra.md) | Pending |
| 4 | [04-firmware.md](04-firmware.md) | Pending |
| 5 | [05-docs.md](05-docs.md) | Pending |
| 6 | [06-scripts.md](06-scripts.md) | Pending |

## Safety & Privacy

- Only whitelisted surface features (glasses, hat, facial hair, expression) are ever sent to the model
- Images are NOT stored — deleted within 1 hour via S3 lifecycle
- No face database, no PII collection
- Signage copy: *"You're being roasted by AI. Faces are processed in real time and not stored."*
