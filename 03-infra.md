# Phase 3: Infrastructure (AWS SAM)

## Goal

Define all AWS resources as code with one-command deploy and teardown.

## Tasks

### 3.1 `infra/template.yaml` — SAM template

Resources:

#### S3 Bucket (image scratch)
```yaml
RoastImagesBucket:
  Type: AWS::S3::Bucket
  Properties:
    BucketName: !Sub roast-images-${AWS::AccountId}-${AWS::Region}
    LifecycleConfiguration:
      - Rule:
          Status: Enabled
          ExpirationInDays: 1
    PublicAccessBlockConfiguration:
      BlockPublicAcls: true
      BlockPublicPolicy: true
      IgnorePublicAcls: true
      RestrictPublicBuckets: true
    Tags:
      - Key: Project
        Value: ai-roast
```

#### IAM Role (Lambda execution)
```yaml
RoastLambdaRole:
  Type: AWS::IAM::Role
  Properties:
    AssumeRolePolicyDocument:
      Version: '2012-10-17'
      Statement:
        - Effect: Allow
          Principal:
            Service: lambda.amazonaws.com
          Action: sts:AssumeRole
    Policies:
      - PolicyName: RoastLambdaPolicy
        PolicyDocument:
          Version: '2012-10-17'
          Statement:
            # CloudWatch logs
            - Effect: Allow
              Action:
                - logs:CreateLogGroup
                - logs:CreateLogStream
                - logs:PutLogEvents
              Resource: arn:aws:logs:*:*:*
            # Rekognition
            - Effect: Allow
              Action: rekognition:DetectFaces
              Resource: '*'
            # Bedrock
            - Effect: Allow
              Action:
                - bedrock:InvokeModel
                - bedrock:InvokeModelWithResponseStream
              Resource:
                - arn:aws:bedrock:*::*:foundation-model/anthropic.claude-*
            # Guardrails
            - Effect: Allow
              Action:
                - bedrock:GetGuardrail
                - bedrock:ApplyGuardrail
              Resource: !Sub arn:aws:bedrock:${AWS::Region}:${AWS::AccountId}:guardrail/*
            # Polly
            - Effect: Allow
              Action: polly:SynthesizeSpeech
              Resource: '*'
            # S3
            - Effect: Allow
              Action:
                - s3:PutObject
                - s3:GetObject
                - s3:DeleteObject
              Resource: !Sub arn:aws:s3:::${RoastImagesBucket}/*
    Tags:
      - Key: Project
        Value: ai-roast
```

#### Lambda Function
```yaml
RoastFunction:
  Type: AWS::Lambda::Function
  Properties:
    FunctionName: ai-roast
    Runtime: python3.12
    Handler: backend.handler.lambda_handler
    CodeUri: ../
    Timeout: 60
    MemorySize: 512
    Environment:
      Variables:
        AWS_REGION: !Ref AWS::Region
        BEDROCK_MODEL_ID: anthropic.claude-sonnet-4-20250514-v1:0
        POLLY_VOICE: Matthew
        ROAST_API_KEY: !Ref RoastApiKey
        IMAGE_BUCKET: !Ref RoastImagesBucket
    Role: !GetAtt RoastLambdaRole.Arn
    Tags:
      Project: ai-roast
```

#### Lambda Function URL (response streaming)
```yaml
RoastFunctionUrl:
  Type: AWS::Lambda::FunctionUrl
  Properties:
    FunctionName: !GetAtt RoastFunction.FunctionName
    AuthorizationType: NONE  # API key handled in-function
    Cors:
      AllowOrigins: ['*']
      AllowMethods: ['POST']
      MaxAge: 86400
```

#### CloudWatch Log Group with retention
```yaml
RoastLogGroup:
  Type: AWS::Logs::LogGroup
  Properties:
    LogGroupName: /aws/lambda/ai-roast
    RetentionInDays: 30
```

#### API Key (parameter)
```yaml
Parameters:
  RoastApiKey:
    Type: String
    NoEcho: true
    Description: API key for authenticating roast requests
```

### 3.2 `infra/params.yaml` — Deployment parameters

```yaml
RoastApiKey: changeme-use-openssl-or-vault
```

### 3.3 Deploy script

`scripts/deploy.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
cd infra
sam build --template template.yaml
sam deploy \
  --template-file template.yaml \
  --stack-name ai-roast \
  --resolve-s3 \
  --parameter-overrides $(cat params.yaml | tr '\n' ' ') \
  --capabilities CAPABILITY_IAM \
  --no-confirm-changeset \
  --no-fail-on-empty-changeset
echo "Deployed. Function URL: $(sam list stack-cloudformation-outputs --stack-name ai-roast --query 'Outputs[?OutputKey==`RoastFunctionUrl`].OutputValue' --output text)"
```

### 3.4 Teardown script

`scripts/teardown.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
# Empty S3 bucket first (SAM can't delete non-empty buckets)
aws s3 rb s3://roast-images-$AWS_ACCOUNT_ID-$AWS_REGION --force 2>/dev/null || true
aws cloudformation delete-stack --stack-name ai-roast
echo "Waiting for stack deletion..."
aws cloudformation wait stack-delete-complete --stack-name ai-roast
echo "Torn down."
```

## Verification

- `sam build` succeeds
- `sam deploy` creates all resources
- `sam delete` (or teardown script) cleans up everything
- IAM policy has no wildcards except where required (Bedrock foundation model ARN pattern)
- S3 bucket has lifecycle rule (1 day) and public access blocked
- Log group has 30-day retention

## Dependencies

- AWS CLI v2
- AWS SAM CLI
- AWS account with Free Tier credits
- Bedrock model access enabled in target region

## Estimated Effort

~2 hours (template writing + deploy verification).
