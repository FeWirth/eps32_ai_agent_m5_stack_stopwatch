"""Render German or English Hermes replies as WAV on the Raspberry Pi."""

import argparse
from pathlib import Path
import wave

import langid
from piper import PiperVoice

VOICES = {"de": "de_DE-jarvis-high", "en": "en_GB-jarvis-high"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("text_file", type=Path)
    parser.add_argument("audio", type=Path)
    args = parser.parse_args()
    text = args.text_file.read_text(encoding="utf-8").strip()
    if not text:
        parser.error("text must not be empty")

    langid.set_languages(list(VOICES))
    language, _ = langid.classify(text)
    model = Path(__file__).resolve().parent / "voices" / f"{VOICES[language]}.onnx"
    voice = PiperVoice.load(str(model))
    with wave.open(str(args.audio), "wb") as output:
        voice.synthesize_wav(text, output)


if __name__ == "__main__":
    main()
