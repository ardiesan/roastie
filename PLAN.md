# AI Roast Device — Project Plan

## Mission

Build an **AI Roast device**: an ESP32-S3 camera that detects a face on-stage, sends it to AWS Lambda, which calls Rekognition → Bedrock (Claude) → Polly to synthesize and return a short, witty, safe spoken roast through a speaker. Built for a live demo at an AWS Community Day — reliability, safety, and "nothing embarrassing on stage" are first-class requirements.

## Architecture

```
[ ESP32-S3 CAM ]                                   ┌─────────────────────────────┐
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

## Assumptions (Recommended Defaults)

| Decision | Default | Rationale |
|----------|---------|-----------|
| Board | ESP32-S3 (Freenove / XIAO ESP32S3 Sense) | GPIO headroom for camera + I2S + Wi-Fi; PSRAM for frame buffer |
| Framework | ESP-IDF v5.x | Best ESP-WHO support |
| Region | us-east-1 | Default AWS region, all services available |
| Bedrock model | Claude Haiku (fast, low-cost) | Sufficient for 20-word roasts |
| Polly voice | Matthew (neural English, male) | Natural-sounding, free-tier eligible |
| IaC | AWS SAM | Simple one-command deploy/teardown |
| Lambda runtime | Python 3.12 | Rich AWS SDK support, easy mocking for tests |
| Audio path | 16 kHz raw PCM (16-bit mono) | No on-device decoder needed; straight to I2S |
| Auth | API key in HTTP header | Simple, sufficient for demo; no IAM complexity |
| Roast tone | G-rated, friendly tech-conf | Safe for diverse audience |
| Image retention | 1 hour via S3 lifecycle | Privacy-first, minimal cost |
| Log retention | 30 days | Enough for post-demo review, not infinite cost |

## Phased Task Plan

| Phase | Doc | Description |
|-------|-----|-------------|
| 1 | [01-scaffold.md](01-scaffold.md) | Monorepo structure, secrets.example.h, gitignore, README skeleton |
| 2 | [02-backend.md](02-backend.md) | Lambda agent loop, safety module, prompts, unit tests |
| 3 | [03-infra.md](03-infra.md) | SAM template with all resources, log retention, S3 lifecycle |
| 4 | [04-firmware.md](04-firmware.md) | ESP-IDF project with WiFiManager, ESP-WHO, I2S playback, offline fallback |
| 5 | [05-docs.md](05-docs.md) | Wiring/BOM, demo runbook, privacy doc, architecture doc |
| 6 | [06-scripts.md](06-scripts.md) | Warm-up script, fallback clip synth, flash helper |
| — | [README.md](README.md) | End-to-end setup, deploy, flash, run, teardown |

## Definition of Done

- [ ] End-to-end happy path: face → roast audio plays, target latency under ~3s warm
- [ ] Every failure branch produces graceful, safe output (verified by tests)
- [ ] Safety verified: no disallowed attributes ever surface
- [ ] `sam deploy` and `sam delete` run cleanly
- [ ] No secrets in git; README lets a stranger reproduce the build
