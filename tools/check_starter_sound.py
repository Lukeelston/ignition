"""Compile and run portable starter audio tests; does not touch hardware."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def linux(path):
    return subprocess.check_output(
        ["wsl.exe", "--exec", "wslpath", "-a", str(path)], text=True
    ).strip()


with tempfile.TemporaryDirectory(prefix="ignition-starter-") as directory:
    source = ROOT / "tests" / "starter_sound.cpp"
    binary = Path(directory) / "starter-check"
    if os.name == "nt":
        source, binary = linux(source), linux(binary)
        prefix = ["wsl.exe", "--exec"]
    else:
        source, binary = str(source), str(binary)
        prefix = []
    subprocess.run(prefix + ["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
                             source, "-o", binary], check=True)
    subprocess.run(prefix + [binary], check=True)
