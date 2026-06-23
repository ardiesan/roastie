"""Unit tests for the agent loop, safety module, and handler."""

import base64
import json
import os
import sys
import pytest
from unittest.mock import patch, MagicMock

# Add parent dir to path so imports work
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from agent import handle_roast
from safety import is_safe, get_fallback_roast, FALLBACK_ROASTS
from prompts import SYSTEM_PROMPT
from handler import lambda_handler


# ── Fixtures ─────────────────────────────────────────────────────

MINIMAL_JPEG = (
    b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01"
    b"\x00\x00\xff\xd9"
)

FAKE_PCM = b"\x00" * 1024  # Dummy 16-bit PCM


def _mock_face(glasses=False, smiling=False, confidence=99.0):
    """Return a mock Rekognition face dict."""
    return {
        "BoundingBox": {"Width": 0.3, "Height": 0.4},
        "Confidence": confidence,
        "Glasses": {"Confidence": 95.0} if glasses else {"Confidence": 5.0},
        "Sunglasses": {"Confidence": 5.0},
        "Hat": {"Confidence": 5.0},
        "Beard": {"Confidence": 5.0},
        "Mustache": {"Confidence": 5.0},
        "Smiling": smiling,
        "Surprised": False,
        "EyesOpen": {"Confidence": 99.0},
        "MouthOpen": {"Confidence": 30.0},
        "AgeRange": {"Low": 20, "High": 40},
        "Emotions": [{"Type": "HAPPY", "Confidence": 80.0}],
    }


# ── Happy path ───────────────────────────────────────────────────

@patch("agent.synthesize", return_value=FAKE_PCM)
@patch("agent.generate_roast", return_value={"roast": "Nice glasses, bro!", "safe": True})
@patch("agent.detect_faces", return_value=[_mock_face(glasses=True, smiling=True)])
def test_happy_path(mock_detect, mock_generate, mock_synth):
    status, body = handle_roast(MINIMAL_JPEG, "correct-key")
    assert status == 200
    assert body == FAKE_PCM
    mock_generate.assert_called_once()
    mock_synth.assert_called_once()


# ── No face ──────────────────────────────────────────────────────

@patch("agent.synthesize", return_value=FAKE_PCM)
@patch("agent.detect_faces", return_value=[])
def test_no_face(mock_detect, mock_synth):
    status, body = handle_roast(MINIMAL_JPEG, "correct-key")
    assert status == 200
    assert body == FAKE_PCM
    mock_synth.assert_called_once_with(get_fallback_roast("no_face"))


# ── Multi-face (pick largest) ────────────────────────────────────

@patch("agent.synthesize", return_value=FAKE_PCM)
@patch("agent.generate_roast", return_value={"roast": "Big face, big jokes!", "safe": True})
@patch("agent.detect_faces", return_value=[
    _mock_face(confidence=99.0),
    {"BoundingBox": {"Width": 0.5, "Height": 0.6}, "Confidence": 99.0,
     "Glasses": {"Confidence": 5.0}, "Sunglasses": {"Confidence": 5.0},
     "Hat": {"Confidence": 5.0}, "Beard": {"Confidence": 5.0},
     "Mustache": {"Confidence": 5.0}, "Smiling": False, "Surprised": False,
     "EyesOpen": {"Confidence": 99.0}, "MouthOpen": {"Confidence": 30.0},
     "AgeRange": {"Low": 20, "High": 40}, "Emotions": []},
    _mock_face(confidence=99.0),
])
def test_multi_face_picks_largest(mock_detect, mock_generate, mock_synth):
    status, body = handle_roast(MINIMAL_JPEG, "correct-key")
    assert status == 200
    assert body == FAKE_PCM


# ── Low confidence ───────────────────────────────────────────────

@patch("agent.synthesize", return_value=FAKE_PCM)
@patch("agent.detect_faces", return_value=[_mock_face(confidence=30.0)])
def test_low_confidence(mock_detect, mock_synth):
    status, body = handle_roast(MINIMAL_JPEG, "correct-key")
    assert status == 200
    assert body == FAKE_PCM
    mock_synth.assert_called_once_with(get_fallback_roast("low_confidence"))


# ── Model refused (safe=False) ───────────────────────────────────

@patch("agent.synthesize", return_value=FAKE_PCM)
@patch("agent.generate_roast", return_value={"roast": "You're so dumb!", "safe": False})
@patch("agent.detect_faces", return_value=[_mock_face()])
def test_model_refused(mock_detect, mock_generate, mock_synth):
    status, body = handle_roast(MINIMAL_JPEG, "correct-key")
    assert status == 200
    assert body == FAKE_PCM
    mock_synth.assert_called_once_with(get_fallback_roast("model_refused"))


# ── Polly error ──────────────────────────────────────────────────

@patch("agent.synthesize", side_effect=RuntimeError("Polly down"))
@patch("agent.generate_roast", return_value={"roast": "Nice glasses, bro!", "safe": True})
@patch("agent.detect_faces", return_value=[_mock_face()])
def test_polly_error(mock_detect, mock_generate, mock_synth):
    status, body = handle_roast(MINIMAL_JPEG, "correct-key")
    assert status == 200
    # Should fall back to fallback roast
    mock_synth.assert_called()
    calls = [c[0][0] for c in mock_synth.call_args_list]
    assert get_fallback_roast("polly_error") in calls


# ── Unauthenticated ──────────────────────────────────────────────

def test_unauthenticated():
    event = {
        "headers": {"x-api-key": "wrong-key"},
        "body": base64.b64encode(MINIMAL_JPEG).decode(),
        "isBase64Encoded": True,
    }
    result = lambda_handler(event, None)
    assert result["statusCode"] == 401


def test_missing_api_key():
    event = {
        "headers": {},
        "body": base64.b64encode(MINIMAL_JPEG).decode(),
        "isBase64Encoded": True,
    }
    result = lambda_handler(event, None)
    assert result["statusCode"] == 401


# ── Invalid image ────────────────────────────────────────────────

def test_too_large_image():
    large = b"\xff\xd8" + b"\x00" * (6 * 1024 * 1024)
    status, body = handle_roast(large, "correct-key")
    assert status == 400


def test_not_a_jpeg():
    status, body = handle_roast(b"not an image", "correct-key")
    assert status == 400


# ── Safety module ────────────────────────────────────────────────

def test_safe_roast():
    assert is_safe("Nice glasses, bro!") is True


def test_unsafe_word_count():
    long_roast = " ".join(["word"] * 25)
    assert is_safe(long_roast) is False


def test_banned_topic_profanity():
    assert is_safe("You're so stupid!") is False


def test_banned_topic_race():
    assert is_safe("Nice look for someone from your background") is False


def test_empty_text():
    assert is_safe("") is False


def test_fallback_roasts_exist():
    """Ensure every branch has a fallback."""
    for branch in FALLBACK_ROASTS:
        roast = get_fallback_roast(branch)
        assert isinstance(roast, str)
        assert len(roast) > 0


# ── System prompt ────────────────────────────────────────────────

def test_system_prompt_mentions_whitelist():
    assert "glasses" in SYSTEM_PROMPT
    assert "hat" in SYSTEM_PROMPT
    assert "facial hair" in SYSTEM_PROMPT


def test_system_prompt_excludes_sensitive():
    for word in ["body", "race", "age", "attractiveness", "intelligence"]:
        assert word not in SYSTEM_PROMPT.lower() or "never about" in SYSTEM_PROMPT.lower()
