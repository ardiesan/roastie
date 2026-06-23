# Architecture

## System Overview

```
  ┌──────────────────────┐          ┌──────────────────────────────────┐
  │   ESP32 Firmware     │          │       AWS Cloud (Lambda)         │
  │                      │          │                                  │
  │  [Camera] OV2640     │──JPEG──►│  [API Gateway / Lambda URL]      │
  │  [I2S]    MAX98357A  │◄──PCM──│  [Lambda: Python 3.12]           │
  │  [Status] LED GPIO 2 │          │                                  │
  │                      │          │   ┌────────────────────────┐    │
  │  Loop:               │          │   │  Agent Brain           │    │
  │    1. Wait WiFi      │          │   │  ├─ Rekognition        │    │
  │    2. Capture JPEG   │          │   │  ├─ Bedrock (Claude)   │    │
  │    3. POST to Lambda │          │   │  ├─ Safety gate        │    │
  │    4. Play PCM audio │          │   │  └─ Polly              │    │
  │    5. Wait 3 s       │          │   └────────────────────────┘    │
  └──────────────────────┘          └──────────────────────────────────┘
```

## Data Flow

1. **Capture:** ESP32 captures a VGA (or QVGA) JPEG from the OV2640 camera stored in PSRAM.
2. **Upload:** HTTP POST to the Lambda Function URL with the JPEG as `application/octet-stream` and `X-API-Key` header.
3. **Perceive (cloud):** Lambda calls Amazon Rekognition `DetectFaces` with `Attributes=['ALL']` to extract face attributes.
4. **Decide (cloud):** Agent selects the largest face, checks confidence, determines branch (single face / no face / multi-face / low confidence).
5. **Reason (cloud):** Lambda calls Amazon Bedrock (Claude Haiku) with a whitelisted feature summary. The model returns JSON: `{"roast": "...", "safe": true/false}`.
6. **Safety gate (cloud):** Code-level checks: word count ≤ 20, banned topics regex, profanity filter. If unsafe → fallback roast.
7. **Act (cloud):** Amazon Polly synthesises the roast text to 16 kHz mono 16-bit PCM audio.
8. **Return:** Lambda returns the PCM audio as base64-encoded bytes with `Content-Type: audio/l16; rate=16000; channels=1`.
9. **Playback:** ESP32 writes the PCM samples directly to I2S (no decoder needed), driving the MAX98357A DAC and speaker.

## Component Details

### Firmware (ESP32)

| Module | File | Responsibility |
|--------|------|---------------|
| WiFi | `wifi.c/h` | Connect to Wi-Fi, store credentials in NVS, stability tracking |
| Camera | `camera.c/h` | OV2640 init via `esp_camera`, JPEG capture to PSRAM |
| HTTP Client | `http_client.c/h` | HTTPS POST to Lambda, receive PCM response |
| I2S Audio | `i2s_audio.c/h` | I2S master mode, 16 kHz / 16-bit / mono → MAX98357A |
| Status | `status.c/h` | LED indicator for system state |
| Face Detect | `face_detect.c/h` | Stability tracking (cloud performs actual detection) |

### Cloud (AWS Lambda)

| Module | File | Responsibility |
|--------|------|---------------|
| Handler | `handler.py` | Lambda entry point, auth check, image decode, response formatting |
| Agent | `agent.py` | Orchestrator: validate → perceive → decide → reason → safety → act |
| Rekognition | `rekognition.py` | Face detection and attribute extraction |
| Bedrock | `bedrock.py` | Claude roast generation via Bedrock runtime |
| Polly | `polly.py` | Speech synthesis to 16 kHz PCM |
| Safety | `safety.py` | Banned topics regex, word count cap, fallback roasts |
| Prompts | `prompts.py` | System prompt and user message builder |

### Infrastructure (AWS SAM)

| Resource | Purpose |
|----------|---------|
| S3 Bucket | Optional image scratch storage with 1-day lifecycle |
| Lambda Function | Python 3.12, 512 MB, 60 s timeout |
| Function URL | Direct HTTPS endpoint (no auth — API key handled in-code) |
| IAM Role | Least-privilege: Rekognition, Bedrock, Polly, S3, CloudWatch |
| Log Group | 30-day retention |

## Safety Design

- **Whitelisted attributes only:** glasses, hat, facial hair, smiling, surprised. No age, race, weight, attractiveness, or any sensitive trait.
- **Bedrock Guardrails:** (optional, configurable) Content filters for hate, insults, sexual content.
- **Code-level safety:** Word count ≤ 20, banned topics regex, profanity filter.
- **Fallback roasts:** Every error branch has a pre-baked safe fallback line.
- **Model refusal handling:** If Bedrock returns `safe: false`, the fallback roast is used instead.

## Privacy Design

- Images are processed in-memory in Lambda (no persistent storage by default).
- Optional S3 scratch storage has a 1-day lifecycle rule.
- No face data or images are retained after processing.
- No face database or recognition — only detection and attribute extraction.

## Latency Budget

| Stage | Target | Notes |
|-------|--------|-------|
| WiFi + capture | ~200 ms | JPEG from OV2640 |
| HTTPS upload | ~300 ms | ~20–50 KB JPEG, depends on signal |
| Lambda cold start | ~2–5 s | First call after idle; use warmup.sh |
| Lambda warm invoke | ~500 ms | Rekognition + Bedrock + Polly |
| PCM download | ~100 ms | ~128 KB for 4 s audio |
| I2S playback | real-time | 16 kHz, streaming |
| **Total (warm)** | **~1.5–2.5 s** | From button press to audio start |
| **Total (cold)** | **~5–8 s** | First roast after idle |
