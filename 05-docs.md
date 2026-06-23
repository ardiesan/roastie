# Phase 5: Documentation

## Goal

Write all documentation: wiring/BOM, architecture, demo runbook, and privacy policy.

## Tasks

### 5.1 `docs/WIRING.md` — Wiring & BOM

**Parts List:**

| Part | Qty | Notes | Estimated Cost |
|------|-----|-------|----------------|
| ESP32-S3-WROOM CAM board (e.g. Freenove ESP32-S3-WROOM-CAM or XIAO ESP32S3 Sense) | 1 | Must have PSRAM + camera connector | $15-25 |
| MAX98357A I2S DAC breakout board | 1 | 3.3V logic, class D amplifier | $5-8 |
| 4Ω or 8Ω speaker (mini) | 1 | 0.5W-1W, 30mm diameter | $3-5 |
| Jumper wires (female-female, male-female) | 1 pack | For breadboard wiring | $3 |
| Breadboard (optional) | 1 | For prototyping | $5 |
| Micro USB cable | 1 | For flashing + power | $2 |

**Wiring (ESP32-S3 → MAX98357A):**

| ESP32-S3 Pin | MAX98357A Pin | Signal |
|--------------|---------------|--------|
| 3.3V | VIN | Power |
| GND | GND | Ground |
| GPIO 17 | BCLK | I2S Bit Clock |
| GPIO 16 | LRC (BCLK on module) | I2S Left/Right Clock |
| GPIO 15 | DIN (SDOUT on module) | I2S Data In |
| GND | GND | Ground |
| 3.3V | FAULT (optional) | Leave floating or pull-up |
| 3.3V | L/R (optional) | Connect to 3.3V for 16-bit mode |

**Speaker wiring:**
- Speaker + → MAX98357A SPK+
- Speaker - → MAX98357A SPK-

### 5.2 `docs/ARCHITECTURE.md` — System architecture

Full architecture document covering:
- High-level data flow diagram
- Firmware architecture (task diagram, state machine)
- Cloud architecture (Lambda flow, service interactions)
- Safety defense-in-depth layers
- Cost estimation per roast and for the day
- Latency breakdown (WiFi round-trip + Rekognition + Bedrock + Polly)
- Failure modes and recovery

### 5.3 `docs/DEMO_RUNBOOK.md` — Demo runbook

**Pre-Show Checklist:**
- [ ] Board charged (battery or USB power bank)
- [ ] WiFi credentials in `secrets.h`, firmware flashed
- [ ] Cloud deployed (`sam deploy`), Lambda Function URL known
- [ ] Lambda warmed up (run `scripts/warmup.sh`)
- [ ] Test on venue WiFi (captive portal check)
- [ ] Face detection working (serial monitor shows "face detected")
- [ ] Roast plays through speaker (happy path verified)
- [ ] Fallback mode tested (disconnect WiFi → local clip plays)
- [ ] Status LED transitions visible

**Failure Playbook:**
| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No face detected | Bad lighting, model not loaded | Check serial, adjust lighting, reboot |
| Upload fails | WiFi isolation, wrong endpoint | Check `secrets.h`, retry |
| No audio | I2S wiring, MAX98357A not powered | Check wiring, verify 3.3V |
| Lambda cold start | First roast after idle | Run warm-up ping, or wait ~30s |
| Roast is empty | Bedrock refused, Guardrail blocked | Check CloudWatch logs, review safety |

**Post-Show Teardown:**
- [ ] Note Lambda Function URL for future reference
- [ ] Save CloudWatch logs if needed for review
- [ ] `scripts/teardown.sh` to clean up AWS resources
- [ ] Unplug device, pack components

### 5.4 `docs/PRIVACY.md` — Privacy policy

**Data flow:**
1. ESP32 captures JPEG of face (stored temporarily in RAM/PSRAM)
2. JPEG sent over HTTPS to Lambda
3. Lambda processes image through Rekognition → Bedrock → Polly
4. **Image is NOT stored** (S3 scratch deleted within 1 hour, or deleted immediately after processing)
5. Audio PCM sent back over HTTPS to device
6. Audio played through speaker, not stored
7. No face data retained. No face database. No long-term storage.

**Signage copy:**
> "📸 You're being roasted by AI! Faces are processed in real time and never stored. Your photo disappears the moment the roast is spoken."

**Compliance notes:**
- No biometric data stored or transmitted to third parties
- Rekognition attributes are ephemeral (in-memory only)
- Bedrock prompts contain only whitelisted surface features (glasses, hat, etc.)
- No PII collected beyond what's visible in a face
- S3 bucket has public access blocked + lifecycle deletion
