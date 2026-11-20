"""Resize the selected artwork with macOS ColorSync and pack the frame as RGB565."""

from pathlib import Path
import struct
import subprocess

SIZE = 466
HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
SOURCE = HERE.parent / "concepts/illustrationen/02-aquarell-bibliothek.png"
SRGB = "/System/Library/ColorSync/Profiles/sRGB Profile.icc"


def main():
    BUILD.mkdir(exist_ok=True)
    bitmap = BUILD / "source.bmp"
    subprocess.run(
        ["sips", "-m", SRGB, "-z", str(SIZE), str(SIZE), "-s", "format",
         "bmp", str(SOURCE), "--out", str(bitmap)],
        check=True,
    )
    data = bitmap.read_bytes()
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height, planes, bits, compression = struct.unpack_from("<iiHHI", data, 18)
    if data[:2] != b"BM" or (width, abs(height), planes, bits, compression) != (
        SIZE, SIZE, 1, 24, 0
    ):
        raise ValueError("Expected a 466 × 466 uncompressed 24-bit BMP")
    stride = (SIZE * 3 + 3) & ~3
    if len(data) < offset + stride * SIZE:
        raise ValueError("Incomplete BMP pixel data")

    with (BUILD / "portrait_pixels.inc").open("w", encoding="ascii") as out:
        for y in range(SIZE):
            row = offset + (y if height < 0 else SIZE - 1 - y) * stride
            for x in range(SIZE):
                dx, dy = 2 * x + 1 - SIZE, 2 * y + 1 - SIZE
                pixel = 0
                if dx * dx + dy * dy <= SIZE * SIZE:
                    start = row + x * 3
                    blue, green, red = data[start:start + 3]
                    pixel = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
                out.write(f"0x{pixel:04x},")
                if x % 16 == 15 or x == SIZE - 1:
                    out.write("\n")


if __name__ == "__main__":
    main()
