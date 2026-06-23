"""AWS Bedrock wrapper for Claude roast generation."""

import json
import os
import boto3
from botocore.exceptions import ClientError

from prompts import SYSTEM_PROMPT, build_user_message

bedrock = boto3.client("bedrock-runtime")
MODEL_ID = os.getenv("BEDROCK_MODEL_ID", "anthropic.claude-haiku-4-20250312-v1:0")


def generate_roast(feature_summary: str) -> dict:
    """Call Bedrock to generate a roast. Returns {"roast": str, "safe": bool}.

    On model refusal or parse error, returns {"roast": "", "safe": False}.
    """
    user_message = build_user_message(feature_summary)

    try:
        response = bedrock.invoke_model(
            modelId=MODEL_ID,
            body=json.dumps({
                "system": SYSTEM_PROMPT,
                "messages": [{"role": "user", "content": user_message}],
                "max_tokens": 500,
                "temperature": 0.9,
            }),
        )
    except ClientError as e:
        raise RuntimeError(f"Bedrock invoke_model failed: {e}")

    body = json.loads(response["body"].read())
    text = body.get("content", [{}])[0].get("text", "")

    # Parse JSON from Claude's response
    try:
        result = json.loads(text)
        return {"roast": result.get("roast", ""), "safe": result.get("safe", False)}
    except (json.JSONDecodeError, KeyError):
        return {"roast": "", "safe": False}
