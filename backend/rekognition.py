"""AWS Rekognition wrapper for face detection."""

import boto3
from botocore.exceptions import ClientError

rekognition = boto3.client("rekognition")


def detect_faces(image_bytes: bytes) -> list:
    """Call Rekognition DetectFaces and return filtered face data.

    Only whitelisted attributes are returned (see safety.py SAFE_ATTRIBUTES).
    Returns a list of face dicts, each with bounding box and filtered attributes.
    """
    try:
        response = rekognition.detect_faces(
            Image={"Bytes": image_bytes},
            Attributes=["ALL"],
        )
    except ClientError as e:
        raise RuntimeError(f"Rekognition DetectFaces failed: {e}")

    faces = []
    for face in response["FaceDetails"]:
        filtered_attrs = {
            k: v
            for k, v in face.items()
            if k in [
                "BoundingBox",
                "AgeRange",
                "Confidence",
                "Emotions",
                "Glasses",
                "Sunglasses",
                "Beard",
                "Mustache",
                "EyesOpen",
                "MouthOpen",
                "Smiling",
                "Surprised",
                "Hat",
            ]
        }
        faces.append(filtered_attrs)

    return faces
