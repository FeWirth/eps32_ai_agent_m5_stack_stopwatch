"""Hermes command STT provider for German or English StopWatch audio on the Raspberry Pi."""

import argparse
from pathlib import Path

from faster_whisper import WhisperModel
from faster_whisper.audio import decode_audio


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audio", type=Path)
    parser.add_argument("transcript", type=Path)
    args = parser.parse_args()

    model = WhisperModel(
        str(Path(__file__).resolve().parent / "models" / "base"),
        device="cpu",
        compute_type="int8",
        cpu_threads=4,
        local_files_only=True,
    )
    audio = decode_audio(str(args.audio))
    if not audio.size:
        raise SystemExit("Keine Audiodaten")
    _, _, probabilities = model.detect_language(audio=audio, vad_filter=True)
    supported = [(language, score) for language, score in probabilities if language in {"de", "en"}]
    language = max(supported, key=lambda candidate: candidate[1])[0]
    segments, _ = model.transcribe(audio, language=language, beam_size=3, vad_filter=True)
    text = " ".join(segment.text.strip() for segment in segments).strip()
    if not text:
        raise SystemExit("Keine Sprache erkannt")
    args.transcript.write_text(text + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
