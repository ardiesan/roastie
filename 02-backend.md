# Phase 2: Backend

## Goal

Implement the Lambda agent loop with safety gates, unit tests, and all fallback branches.

## Architecture

```
handler.py (entry point, HTTP parsing, auth check)
    └── agent.py (orchestrator: perceive → decide → reason → safety → act)
            ├── rekognition.py (DetectFaces wrapper)
            ├── bedrock.py (Claude + Guardrails wrapper)
            ├── polly.py (SynthesizeSpeech wrapper)
            ├── safety.py (whitelist, profanity, word count, fallback roasts)
            └── prompts.py (system prompt templates)
```

## Tasks

### 2.1 `backend/requirements.txt`

```
boto3>=1.34.0
botocore>=1.34.0
```

### 2.2 `backend/safety.py` — Safety module

- `SAFE_ATTRIBUTES`: whitelist = `["glasses", "sunglasses", "hat", "cap", "facial_hair", "smiling", "surprised", "calm"]`
- `is_safe(roast_text: str) -> bool`: checks word count <= 20, no banned topics (regex-based), no profanity
- `get_fallback_roast(branch: str) -> str`: returns pre-baked roasts for each branch:
  - `"no_face"`: "I can't roast what I can't see... try standing in the light!"
  - `"multi_face"`: "You're all a group photo — pick one and come back!"
  - `"low_confidence"`: "The lighting's too tricky — step closer for a proper roast!"
  - `"model_refused"`: "Even AI thinks that roast would be too mean. Try again!"
  - `"guardrail_block"`: "That roast didn't pass the safety filter. Let me try something nicer!"
  - `"polly_error"`: "My voice box is acting up — check back in a sec!"
  - `"general_error"`: "Something went wrong on my end. Roast denied! (Try again?)"

### 2.3 `backend/prompts.py` — System prompt

```python
SYSTEM_PROMPT = """You are a playful, good-natured comedian doing light, affectionate roasts at a tech conference.
You receive a short list of approved, surface-level visual details (e.g. glasses, hat, facial hair, expression).
Write ONE witty, gentle roast under 20 words using ONLY those details.
The vibe is a friendly jab between colleagues — never mean, never personal, never about body, race, age,
attractiveness, intelligence, or any sensitive trait.
If you have nothing safe to say, make a harmless joke about the situation instead.
Output JSON: {{"roast": "...", "safe": true|false}}"""
```

### 2.4 `backend/rekognition.py` — Rekognition wrapper

- `detect_faces(image_bytes: bytes) -> dict`: calls `rekognition.detect_faces(Attributes=['ALL'], Image={'Bytes': image_bytes})`
- Returns parsed face list with attributes filtered to whitelist only

### 2.5 `backend/bedrock.py` — Bedrock wrapper

- `generate_roast(feature_summary: str) -> dict`: calls `bedrock_runtime.invoke_model()` with:
  - Model: `anthropic.claude-sonnet-4-20250514-v1:0` (or Haiku)
  - Body: system prompt + user message with feature summary
  - Returns parsed JSON with `roast` and `safe` fields
- Handles model refusal (returns `{"roast": "", "safe": False}`)

### 2.6 `backend/polly.py` — Polly wrapper

- `synthesize(text: str) -> bytes`: calls `polly.synthesize_voice()` with:
  - Engine: `neural`
  - AudioFormat: `pcm`
  - SampleRate: `16000`
  - VoiceId: `Matthew`
  - Returns raw 16-bit PCM bytes
- On error: returns `None` (caller uses fallback)

### 2.7 `backend/agent.py` — Agent orchestrator

```python
def handle_roast(image_bytes: bytes, api_key: str) -> tuple[int, bytes]:
    # 1. Auth check (called from handler before agent)
    # 2. Validate image (size <= 5MB, valid JPEG)
    # 3. Call Rekognition → get faces + attributes
    # 4. Decide branch:
    #    - no faces → fallback
    #    - multiple → pick largest face by bounding box area
    #    - low confidence → generic fallback
    # 5. Build feature summary from whitelist only
    # 6. Call Bedrock → get JSON roast + safe flag
    # 7. Safety gate: is_safe(roast) → if not, use fallback
    # 8. Call Polly → get PCM bytes
    # 9. On Polly error → use pre-synthesized fallback clip
    # 10. Return (200, pcm_bytes) or (200, fallback_pcm_bytes)
```

### 2.8 `backend/handler.py` — Lambda entry point

- Parse multipart/form-data or raw JPEG from HTTP body
- Check `X-API-Key` header against env var
- Call `agent.handle_roast()`
- Return streaming response with `Content-Type: audio/l16; rate=16000; channels=1`
- Structured logging with `request_id`

### 2.9 `backend/tests/test_agent.py` — Unit tests

Test every branch:
- `test_happy_path`: mock Rekognition (1 face, glasses + smiling) → Bedrock (safe roast) → Polly (PCM) → verify 200 + PCM
- `test_no_face`: mock Rekognition (empty faces) → verify fallback roast
- `test_multi_face`: mock Rekognition (3 faces) → verify largest face picked
- `test_low_confidence`: mock Rekognition (confidence < 0.5) → verify fallback
- `test_model_refused`: mock Bedrock (safe=False) → verify fallback
- `test_guardrail_block`: mock Bedrock (safe=False, guardrail triggered) → verify fallback
- `test_polly_error`: mock Polly (raises) → verify fallback clip
- `test_unauthenticated`: wrong API key → verify 401
- `test_too_large_image`: image > 5MB → verify 400
- `test_not_a_jpeg`: invalid image → verify 400
- `test_profanity_filter`: Bedrock returns profane roast → verify fallback
- `test_word_count_exceeded`: Bedrock returns 25-word roast → verify fallback

## Verification

- `pytest backend/tests/` passes all tests
- Every branch in `agent.py` has at least one test
- Safety module tested with edge-case inputs

## Dependencies

- `boto3`, `botocore` (Python AWS SDK)
- No external services needed for tests (all mocked)

## Estimated Effort

~4-6 hours (implementation + comprehensive tests).
