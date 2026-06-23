# Bedrock Setup — Step by Step

This guide enables Claude model access in Amazon Bedrock for the AI Roast device.

## Prerequisites

- AWS account with Free Tier credits ($200)
- AWS CLI configured with credentials (`aws configure`)
- Region: **us-east-1** (default for this project)

## Step 1: Open the Bedrock Console

1. Go to https://console.aws.amazon.com/bedrock/
2. Confirm your region is **us-east-1** (top-right corner)

## Step 2: Enable Model Access

1. In the left sidebar, click **Model access**
2. Click **Get model access** button
3. Find **Anthropic / Claude Haiku 4 (haiku-4-20250312-v1:0)** in the list
4. Click **Request model access** next to it
5. Check the box to confirm terms of use
6. Click **Submit request**

> **Note:** Model access is typically granted instantly for Free Tier accounts, but can take up to 24 hours.

## Step 3: Verify Access

Run this command to confirm the model is available:

```bash
aws bedrock list-foundation-models \
  --region us-east-1 \
  --by-provider anthropic \
  --output json | grep -E '"modelId"|"status"'
```

You should see `anthropic.claude-haiku-4-20250312-v1:0` with status `AVAILABLE`.

## Step 4: (Optional) Enable Claude Sonnet

If you want a higher-quality roast option, you can also enable Claude Sonnet:

1. Repeat Step 2 for **Claude Sonnet** models
2. Update `BEDROCK_MODEL_ID` in `backend/.env` or the SAM template

## Troubleshooting

- **"Service Quota exceeded"**: You may need to request a quota increase. Go to Service Quotas → AWS Bedrock.
- **Model shows as `UNAVAILABLE`**: Wait a few minutes and retry. The model may still be provisioning.
- **Region not available**: Bedrock is not available in all regions. us-east-1 and us-west-2 are the safest bets.
