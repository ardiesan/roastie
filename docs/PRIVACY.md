# Privacy

## Data Flow

```
[Person] → [Camera: JPEG] → [HTTPS → AWS Lambda] → [Delete JPEG]
                                        │
                            ┌───────────┼───────────┐
                            ▼           ▼           ▼
                         Rekognition  Bedrock      Polly
                         (faces)     (roast)    (speech)
                            │           │           │
                            └─────── PCM audio back to device ───────→ [Speaker]
```

## What We Collect

- **Camera image:** A JPEG photo captured when someone stands in front of the device.
- **Face attributes:** Surface-level visual details extracted by Amazon Rekognition (glasses, hat, facial hair, expression).

## What We Do NOT Collect

- **No face database.** We do not store, match, or track faces across sessions.
- **No personal data.** No names, ages, genders, or identifiers.
- **No photos retained.** Images are processed in-memory and deleted immediately.
- **No audio storage.** Roasts are synthesized and played in real time — never recorded or stored.

## Image Retention

- Images are processed in-memory by the Lambda function.
- If S3 scratch storage is enabled, images are automatically deleted within **1 hour** via an S3 Lifecycle rule.
- CloudWatch logs contain only structured metadata (latency, status codes) — no image data or face attributes.

## AWS Services Used

| Service | Data Processed | Retention |
|---------|---------------|-----------|
| Amazon Rekognition | Face detection attributes | In-memory only, not stored |
| Amazon Bedrock | Roast text generation | In-memory only, not stored |
| Amazon Polly | Speech synthesis | In-memory only, not stored |
| S3 | Optional image scratch | Deleted within 1 hour |
| CloudWatch Logs | Structured logs only | 30 days (configurable) |

## Compliance Notes

- This device performs **real-time face detection only** — no recognition or identification.
- No biometric templates are created or stored.
- The device is intended for **voluntary participation** at a public event.
- Signage should be displayed informing attendees that the AI roast station is active.

## Demo Signage Copy

> **📸 You're being roasted by AI!**
>
> Your face is processed in real time by Amazon AI services.
> No photos, faces, or audio are stored. Everything is deleted immediately.
>
> *Powered by AWS Free Tier.*
