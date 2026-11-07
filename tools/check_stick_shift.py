"""Compile and run portable thumbstick behavior tests; does not touch hardware."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def linux(path):
    return subprocess.check_output(
        ["wsl.exe", "--exec", "wslpath", "-a", str(path)], text=True
    ).strip()


with tempfile.TemporaryDirectory(prefix="ignition-stick-") as directory:
    source = ROOT / "tests" / "stick_shift.cpp"
    binary = Path(directory) / "stick-check"
    if os.name == "nt":
        source, binary = linux(source), linux(binary)
        prefix = ["wsl.exe", "--exec"]
    else:
        source, binary = str(source), str(binary)
        prefix = []
    subprocess.run(prefix + ["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
                             source, "-o", binary], check=True)
    subprocess.run(prefix + [binary], check=True)
