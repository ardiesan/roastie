# Phase 1: Scaffold

## Goal

Create a clean monorepo structure with all directories, shared config files, and a README skeleton.

## Tasks

### 1.1 Create directory structure

```
roast-system/
├── firmware/          # ESP32 firmware (ESP-IDF)
│   └── main/          # Source files
├── backend/           # Lambda code
│   ├── tests/         # Unit tests
│   └── requirements.txt
├── infra/             # AWS SAM template
├── docs/              # Architecture, runbook, wiring, privacy
├── scripts/           # Helpers (warm-up, flash, synth)
├── .gitignore
├── .envrc
├── PLAN.md
└── README.md
```

### 1.2 Create `.gitignore`

Exclude:
- `**/*.bin`, `**/*.elf`, `**/*.map` (ESP-IDF build artifacts)
- `.env`, `secrets.h`, `*.key`, `*.pem` (secrets)
- `__pycache__/`, `*.pyc`, `.pytest_cache/`, `node_modules/` (Python/Node caches)
- `.aws-sam/` (SAM build artifacts)
- `.vscode/`, `.idea/` (IDE configs)
- `*.gguf` (model files — symlink already exists)

### 1.3 Create `secrets.example.h` (firmware)

```c
// secrets.example.h — copy to secrets.h and fill in real values
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-password"
#define ROAST_ENDPOINT "https://xxxxx.execute-api.us-east-1.amazonaws.com/roast"
#define ROAST_API_KEY "your-api-key"

#endif // SECRETS_H
```

### 1.4 Create `backend/.env.example`

```
AWS_REGION=us-east-1
BEDROCK_MODEL_ID=anthropic.claude-sonnet-4-20250514-v1:0
POLLY_VOICE=Matthew
ROAST_API_KEY=your-api-key
IMAGE_BUCKET=roast-images-<unique>
LAMBDA_FUNCTION_NAME=ai-roast
```

### 1.5 Create `infra/events.yaml` (SAM event mapping)

Placeholder for Lambda event source mappings.

### 1.6 Create README skeleton

Write a minimal README with:
- Project title and description
- Architecture diagram (placeholder)
- Links to phase docs
- "Getting Started" section (TBD in Phase 5)

## Verification

- `ls` shows all directories in correct structure
- `.gitignore` excludes expected patterns
- `secrets.example.h` exists and is committed (real `secrets.h` is gitignored)
- `.env.example` exists in backend/

## Dependencies

None — this phase has no external dependencies.

## Estimated Effort

~30 minutes (mostly file creation and review).
