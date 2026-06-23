# Phase 6: Scripts

## Goal

Create helper scripts for deployment, flashing, warm-up, and fallback clip generation.

## Tasks

### 6.1 `scripts/deploy.sh` — Deploy cloud infrastructure

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Generate a random API key if not provided
API_KEY="${ROAST_API_KEY:-$(openssl rand -hex 16)}"
echo "API Key: $API_KEY"
echo "Save this for your firmware secrets.h!"

cd infra
sam build --template-file template.yaml
sam deploy \
  --template-file template.yaml \
  --stack-name ai-roast \
  --resolve-s3 \
  --parameter-overrides RoastApiKey="$API_KEY" \
  --capabilities CAPABILITY_IAM \
  --no-confirm-changeset \
  --no-fail-on-empty-changeset

# Output the function URL
URL=$(sam list stack-cloudformation-outputs --stack-name ai-roast \
  --query "Outputs[?OutputKey=='RoastFunctionUrl'].OutputValue" --output text)
echo ""
echo "=== Deployed ==="
echo "Function URL: $URL"
echo "API Key: $API_KEY"
```

### 6.2 `scripts/teardown.sh` — Destroy cloud infrastructure

```bash
#!/usr/bin/env bash
set -euo pipefail

# Get account ID and region
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
REGION="${AWS_REGION:-us-east-1}"
BUCKET="roast-images-${ACCOUNT_ID}-${REGION}"

# Empty and delete S3 bucket
echo "Deleting S3 bucket: $BUCKET"
aws s3 rb "s3://$BUCKET" --force 2>/dev/null || true

# Delete CloudFormation stack
echo "Deleting CloudFormation stack: ai-roast"
aws cloudformation delete-stack --stack-name ai-roast
aws cloudformation wait stack-delete-complete --stack-name ai-roast
echo "Torn down."
```

### 6.3 `scripts/warmup.sh` — Warm up Lambda

```bash
#!/usr/bin/env bash
set -euo pipefail

ENDPOINT="${1:?Usage: warmup.sh <lambda-url>}"
API_KEY="${ROAST_API_KEY:?Set ROAST_API_KEY env var}"

# Generate a tiny test JPEG (1x1 pixel, minimal valid JPEG)
echo -ne '\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00\xff\xdb\x00C\x00\x08\x06\x06\x07\x06\x05\x08\x07\x07\x07\t\t\x08\n\x0c\x14\r\x0c\x0b\x0b\x0c\x19\x12\x13\x0f\x14\x1d\x1a\x1f\x1e\x1d\x1a\x1c\x1c $.\x27 ",#\x1c\x1c(7teletext69teletext]](9teletext]](9teletext]]9teletext]\xff\xc0\x00\x0b\x08\x00\x01\x00\x01\x01\x01\x11\x00\xff\xc4\x00\x1f\x00\x00\x01\x05\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\xff\xc4\x00\xb5\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05\x05\x04\x04\x00\x00\x01}\x01\x02\x03\x00\x04\x11\x05\x12!1A\x06\x13Qa\x07"q\x142\x81\x91\xa1\x08#B\xb1\xc1\x15R\xd1\xf0$3br\x82\t\n\x16\x17\x18\x19\x1a%&\x27()*456789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz\x83\x84\x85\x86\x87\x88\x89\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xff\xda\x00\x08\x01\x01\x00\x00?\x00\xfb\xd5\xdb\x20\xb8\xae\x5f\xff\xd9' > /tmp/test.jpg

curl -s -o /dev/null -w "HTTP %{http_code} in %{time_total}s\n" \
  -X POST "$ENDPOINT" \
  -H "X-API-Key: $API_KEY" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @/tmp/test.jpg

rm /tmp/test.jpg
```

### 6.4 `scripts/flash.sh` — Flash firmware to ESP32

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PORT="${1:-/dev/ttyUSB0}"

# Check if secrets.h exists
if [ ! -f firmware/main/secrets.h ]; then
  echo "ERROR: firmware/main/secrets.h not found!"
  echo "Copy firmware/main/secrets.example.h and fill in your values."
  exit 1
fi

cd firmware
idf.py -p "$PORT" flash monitor
```

### 6.5 `scripts/synth_fallback.py` — Generate fallback audio clips

```python
"""Generate pre-synthesized fallback audio clips for offline/demo mode."""
import boto3
import sys
import os

VOICE = os.getenv("POLLY_VOICE", "Matthew")
OUTPUT_DIR = sys.argv[1] if len(sys.argv) > 1 else "firmware/assets/audio"

FALLBACK_TEXTS = {
    "no_face": "I can't roast what I can't see. Try standing in the light!",
    "offline_1": "Cloud is down, but I still got jokes. You're doing great!",
    "offline_2": "Even without the internet, you're awesome. Now go fix my WiFi!",
    "offline_3": "Technical difficulties? Classic. At least your outfit is safe.",
}

polly = boto3.client("polly", region_name=os.getenv("AWS_REGION", "us-east-1"))

for name, text in FALLBACK_TEXTS.items():
    response = polly.synthesize_voice(
        Text=text,
        Engine="neural",
        VoiceId=VOICE,
        AudioFormat="pcm",
        SampleRate="16000",
    )
    audio = response["AudioStream"].read()
    path = os.path.join(OUTPUT_DIR, f"{name}.pcm")
    with open(path, "wb") as f:
        f.write(audio)
    print(f"Generated: {path} ({len(audio)} bytes)")
```

### 6.6 `scripts/build_firmware.sh` — Build firmware without flashing

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../firmware"
idf.py build
echo "Build complete. Flash with: scripts/flash.sh [port]"
```

## Verification

- `scripts/deploy.sh` deploys successfully
- `scripts/warmup.sh <url>` returns HTTP 200
- `scripts/flash.sh /dev/ttyUSB0` flashes and starts monitor
- `scripts/teardown.sh` cleans up all resources
- `scripts/synth_fallback.py` generates .pcm files in firmware/assets/audio/

## Dependencies

- `awscli`, `sam-cli`, `idf.py` (ESP-IDF)
- `boto3` (for synth_fallback.py)
- `openssl` (for API key generation)

## Estimated Effort

~1-2 hours (script writing + testing).
