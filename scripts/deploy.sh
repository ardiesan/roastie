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
