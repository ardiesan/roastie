"""Agent orchestrator: perceive → decide → reason → safety → act."""

import os
import json
import base64
from rekognition import detect_faces
from bedrock import generate_roast
from polly import synthesize
from safety import is_safe, get_fallback_roast

IMAGE_BUCKET = os.getenv("IMAGE_BUCKET", "")


def _build_feature_summary(faces: list) -> str:
    """Build a feature summary from the largest face, using only whitelisted attributes."""
    if not faces:
        return ""

    # Pick largest face by bounding box area
    face = max(faces, key=lambda f: (
        f["BoundingBox"]["Width"] * f["BoundingBox"]["Height"]
    ))

    features = []
    if face.get("Glasses", {}).get("Confidence", 0) > 50:
        features.append("wearing glasses")
    if face.get("Sunglasses", {}).get("Confidence", 0) > 50:
        features.append("wearing sunglasses")
    if face.get("Hat", {}).get("Confidence", 0) > 50:
        features.append("wearing a hat")
    if face.get("Beard", {}).get("Confidence", 0) > 50:
        features.append("has a beard")
    if face.get("Mustache", {}).get("Confidence", 0) > 50:
        features.append("has a mustache")
    if face.get("Smiling", False):
        features.append("is smiling")
    if face.get("Surprised", False):
        features.append("looks surprised")

    return ", ".join(features) if features else "no distinctive features detected"


def handle_roast(image_bytes: bytes, api_key: str) -> tuple:
    """Main agent loop. Returns (status_code, audio_bytes)."""
    # 1. Validate image
    if len(image_bytes) > 5 * 1024 * 1024:
        return 400, b"Image too large (max 5MB)"
    if not image_bytes.startswith(b"\xff\xd8"):
        return 400, b"Not a valid JPEG"

    # 2. Save to S3 for debug (optional)
    if IMAGE_BUCKET:
        _save_image(image_bytes)

    # 3. Perceive — Rekognition
    try:
        faces = detect_faces(image_bytes)
    except RuntimeError:
        return 200, synthesize(get_fallback_roast("general_error"))

    # 4. Decide branch
    if not faces:
        fallback = get_fallback_roast("no_face")
        return 200, synthesize(fallback)

    # Pick largest face
    largest = max(faces, key=lambda f: (
        f["BoundingBox"]["Width"] * f["BoundingBox"]["Height"]
    ))

    # Check confidence
    if largest.get("Confidence", 0) < 50:
        fallback = get_fallback_roast("low_confidence")
        return 200, synthesize(fallback)

    # 5. Reason — Bedrock
    feature_summary = _build_feature_summary(faces)
    try:
        result = generate_roast(feature_summary)
    except RuntimeError:
        fallback = get_fallback_roast("general_error")
        return 200, synthesize(fallback)

    # 6. Safety gate
    if not result.get("safe") or not is_safe(result.get("roast", "")):
        fallback = get_fallback_roast("model_refused")
        return 200, synthesize(fallback)

    # 7. Act — Polly
    try:
        audio = synthesize(result["roast"])
        return 200, audio
    except RuntimeError:
        fallback = get_fallback_roast("polly_error")
        return 200, synthesize(fallback)


def _save_image(image_bytes: bytes):
    """Save image to S3 for debug purposes."""
    import uuid
    s3 = boto3.client("s3")
    key = f"roasts/{uuid.uuid4().hex}.jpg"
    s3.put_object(Bucket=IMAGE_BUCKET, Key=key, Body=image_bytes)
