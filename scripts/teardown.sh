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
