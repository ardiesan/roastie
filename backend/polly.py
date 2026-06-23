"""AWS Polly wrapper for speech synthesis."""

import os
import boto3
from botocore.exceptions import ClientError

polly = boto3.client("polly")
VOICE_ID = os.getenv("POLLY_VOICE", "Matthew")


def synthesize(text: str) -> bytes:
    """Synthesize speech and return raw 16-bit PCM bytes.

    Returns None on error (caller should use fallback).
    """
    try:
        response = polly.synthesize_speech(
            Text=text,
            Engine="neural",
            VoiceId=VOICE_ID,
            AudioFormat="pcm",
            SampleRate="16000",
        )
    except ClientError as e:
        raise RuntimeError(f"Polly synthesize_speech failed: {e}")

    return response["AudioStream"].read()
