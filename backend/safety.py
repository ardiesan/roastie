"""Safety module: attribute whitelist, profanity filter, fallback roasts."""

SAFE_ATTRIBUTES = [
    "glasses",
    "sunglasses",
    "hat",
    "cap",
    "facial_hair",
    "smiling",
    "surprised",
    "calm",
]

# Banned topics (regex-based)
BANNED_TOPICS = [
    r"\b(stupid|idiot|fool|dumb|moron|loser|ugly|fat|skinny|old|young)\b",
    r"\b(black|white|asian|hispanic|latino|native)\b",
    r"\b(muslim|jewish|christian|atheist|hindu)\b",
    r"\b(disabled|cripple|retarded|autism|autistic)\b",
    r"\b(gay|lesbian|trans|straight)\b",
    r"\b(body|weight|breast|butt|ass|skin)\b",
]

FALLBACK_ROASTS = {
    "no_face": "I can't roast what I can't see... try standing in the light!",
    "multi_face": "You're all a group photo — pick one and come back!",
    "low_confidence": "The lighting's too tricky — step closer for a proper roast!",
    "model_refused": "Even AI thinks that roast would be too mean. Try again!",
    "guardrail_block": "That roast didn't pass the safety filter. Let me try something nicer!",
    "polly_error": "My voice box is acting up — check back in a sec!",
    "general_error": "Something went wrong on my end. Roast denied! (Try again?)",
}


def is_safe(roast_text: str) -> bool:
    """Check if a roast passes safety gates."""
    if not roast_text or len(roast_text.strip()) == 0:
        return False

    # Word count <= 20
    if len(roast_text.split()) > 20:
        return False

    # Banned topic check
    for pattern in BANNED_TOPICS:
        if pattern.lower() in roast_text.lower():
            return False

    return True


def get_fallback_roast(branch: str) -> str:
    """Return a pre-baked fallback roast for the given branch."""
    return FALLBACK_ROASTS.get(branch, FALLBACK_ROASTS["general_error"])
