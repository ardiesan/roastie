# Demo Runbook

## Pre-Show Checklist

### Day Before

- [ ] Flash latest firmware to device (see `scripts/flash.sh`)
- [ ] Deploy latest cloud stack (see `scripts/deploy.sh`)
- [ ] Run `scripts/warmup.sh` — verify HTTP 200 and latency < 5 s
- [ ] Charge device battery to 100 %
- [ ] Generate and print fallback PCM clips (see `scripts/synth_fallback.py`)
- [ ] Test end-to-end: stand in front of device, verify roast plays
- [ ] Test every failure branch:
  - [ ] No face (point camera at wall) → fallback roast
  - [ ] Multiple faces → group roast or largest face
  - [ ] Low light → low confidence fallback
  - [ ] Wi-Fi disconnect → error blink pattern

### Day Of Show

- [ ] Arrive 1 hour early
- [ ] Connect to venue Wi-Fi, verify internet connectivity
- [ ] Flash device if firmware was updated
- [ ] Run `scripts/warmup.sh` — **must pass before demo starts**
- [ ] Set up device on table with speaker facing audience
- [ ] Position device at ~1–2 m height (easiest for face detection)
- [ ] Ensure battery is plugged in or fully charged
- [ ] Run 5–10 test roasts with team members

## Wi-Fi Notes

Conference Wi-Fi often has **client isolation** (devices can't talk to each other) and **captive portals**.

- If the venue Wi-Fi blocks outbound HTTPS, set up a **mobile hotspot** from your phone as a fallback.
- If a captive portal is required, the device cannot pass through it. Use a hotspot or ask IT for an open network for the demo.
- Store venue Wi-Fi credentials in `firmware/main/secrets.h` and reflash if needed.

## Status LED Guide

| LED Pattern | Meaning | Action |
|-------------|---------|--------|
| Solid off | Idle, waiting | Normal |
| Solid on | Uploading to cloud | Normal |
| Solid on (longer) | Playing roast | Normal |
| Blinking 2× (500 ms) | Wi-Fi not connected | Check Wi-Fi, reconnect |
| Blinking 3× (300 ms) | Capture or upload failed | Check camera, network |
| Blinking 5× (200 ms) | I2S playback failed | Check speaker wiring |

## Failure Playbook

| Problem | Likely Cause | Fix |
|---------|-------------|-----|
| No audio at all | Speaker not connected / I2S pins wrong | Check wiring per WIRING.md |
| Roast never plays | Lambda returning error | Check `scripts/warmup.sh`, verify endpoint |
| "No face" roast every time | Camera pointed wrong / lighting | Reposition camera, improve lighting |
| Device keeps rebooting | Insufficient power | Use 2 A+ USB supply |
| Wi-Fi connects but no data | Client isolation / captive portal | Switch to phone hotspot |
| Lambda cold start every time | Lambda idle (provisioned concurrency off) | Run `scripts/warmup.sh` periodically |
| Crackling audio | I2S buffer underrun | Check DMA buffering, reduce frame rate |

## During Demo

- **Keep the device steady.** Mount it on a stand or heavy base so it doesn't wobble.
- **Announce the privacy note:** "Faces are processed in real time and not stored."
- **If the cloud goes down:** The device will keep capturing and showing error blinks. Switch to offline mode (pre-loaded fallback clips) if available.
- **If Bedrock is slow:** The roast will take longer but will still play. Don't worry — audiences expect a "thinking" pause.

## Post-Show

- [ ] Run `scripts/teardown.sh` to destroy cloud resources (saves cost)
- [ ] Save the `secrets.h` and `params.yaml` for future demos
- [ ] Note any bugs or issues in CHANGELOG.md
- [ ] Recharge the device battery

## Teardown

```bash
# Destroy AWS resources (S3 bucket + CloudFormation stack)
./scripts/teardown.sh

# Optionally delete the CloudFormation stack manually
aws cloudformation delete-stack --stack-name roast-system
```
