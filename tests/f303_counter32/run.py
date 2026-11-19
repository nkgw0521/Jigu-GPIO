import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--compiler", default="gcc")
args = parser.parse_args()
tests = Path(__file__).resolve().parent
repo = tests.parents[1]
with tempfile.TemporaryDirectory(prefix="Jigu-counter32-test-") as tmp:
    tmp = Path(tmp)
    shutil.copy2(repo / "Boards/NUCLEO-F303K8/Core/Inc/counter32.h", tmp)
    shutil.copy2(tests / "mocks/tim.h", tmp)
    exe = tmp / "test_counter32.exe"
    subprocess.run([args.compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I" + str(tmp),
                    str(repo / "Boards/NUCLEO-F303K8/Core/Src/counter32.c"),
                    str(tests / "test_counter32.c"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
