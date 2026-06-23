"""Lambda entry point: HTTP parsing, auth check, agent dispatch."""

import base64
import os
import json
import uuid
from agent import handle_roast

API_KEY = os.getenv("ROAST_API_KEY", "")


def lambda_handler(event, context):
    """AWS Lambda handler for roast requests."""
    request_id = str(uuid.uuid4())[:8]
    print(f"[{request_id}] Request received")

    # Auth check
    headers = event.get("headers", {})
    provided_key = headers.get("x-api-key", "")
    if not provided_key or provided_key != API_KEY:
        return {
            "statusCode": 401,
            "body": json.dumps({"error": "Unauthorized"}),
            "headers": {"Content-Type": "application/json"},
        }

    # Extract image bytes from body
    body = event.get("body", "")
    is_base64 = event.get("isBase64Encoded", False)

    if is_base64:
        image_bytes = bytearray(base64.b64decode(body))
    else:
        image_bytes = bytearray(body, "utf-8") if isinstance(body, str) else bytearray(body)

    # Call agent
    status_code, audio_or_error = handle_roast(image_bytes, provided_key)

    # Return audio response
    if status_code == 200 and not isinstance(audio_or_error, bytes):
        return {
            "statusCode": 500,
            "body": json.dumps({"error": "Internal error"}),
            "headers": {"Content-Type": "application/json"},
        }

    return {
        "statusCode": status_code,
        "body": str(audio_or_error) if isinstance(audio_or_error, bytes) else audio_or_error,
        "isBase64Encoded": True,
        "headers": {
            "Content-Type": "audio/l16; rate=16000; channels=1",
            "X-Request-Id": request_id,
        },
    }
