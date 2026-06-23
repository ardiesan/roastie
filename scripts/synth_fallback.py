"""Generate pre-synthesized fallback audio clips for offline/demo mode."""
import boto3
import sys
import os

VOICE = os.getenv("POLLY_VOICE", "Matthew")
OUTPUT_DIR = sys.argv[1] if len(sys.argv) > 1 else "firmware/assets/audio"

FALLBACK_TEXTS = {
    "no_face": "I can't roast what I can't see. Try standing in the light!",
    "offline_1": "Cloud is down, but I still got jokes. You're doing great!",
    "offline_2": "Even without the internet, you're awesome. Now go fix my WiFi!",
    "offline_3": "Technical difficulties? Classic. At least your outfit is safe.",
}

polly = boto3.client("polly", region_name=os.getenv("AWS_REGION", "us-east-1"))

os.makedirs(OUTPUT_DIR, exist_ok=True)

for name, text in FALLBACK_TEXTS.items():
    response = polly.synthesize_speech(
        Text=text,
        Engine="neural",
        VoiceId=VOICE,
        AudioFormat="pcm",
        SampleRate="16000",
    )
    audio = response["AudioStream"].read()
    path = os.path.join(OUTPUT_DIR, f"{name}.pcm")
    with open(path, "wb") as f:
        f.write(audio)
    print(f"Generated: {path} ({len(audio)} bytes)")
