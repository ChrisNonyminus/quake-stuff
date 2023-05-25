import os
import sys
import argparse

CC = "cc" if os.getenv("CC") is None else str(os.getenv("CC"))
LD = "cc" if os.getenv("LD") is None else str(os.getenv("LD"))
CFLAGS = (
    "-O1 -g -w --std=c9x" if os.getenv("CFLAGS") is None else str(os.getenv("CFLAGS"))
)
LDFLAGS = "-lm -lSDL2 -g" if os.getenv("LDFLAGS") is None else str(os.getenv("LDFLAGS"))


GAMENAME = "QUAKE" if os.getenv("GAMENAME") is None else str(os.getenv("GAMENAME"))

SYS_BACKEND = (
    "SDL" if os.getenv("SYS_BACKEND") is None else str(os.getenv("SYS_BACKEND"))
)

ENGINEVER = "Q1" if os.getenv("ENGINEVER") is None else str(os.getenv("ENGINEVER"))
R_BACKEND = "SOFT" if os.getenv("R_BACKEND") is None else str(os.getenv("R_BACKEND"))

Includes = [
    "-I.",
    f"-I{ENGINEVER.lower()}src/client",
    f"-I{ENGINEVER.lower()}src/server",
    f"-I{ENGINEVER.lower()}src/qcommon",
    f"-I{ENGINEVER.lower()}render/ref_{R_BACKEND.lower()}",
]

if os.getenv("INCLUDES") is not None:
    Includes.append(str(os.getenv("INCLUDES")))

CFlags = [
    CFLAGS,
    f'-DQ_GAME="Q_GAME_{GAMENAME}"',
    " ".join(Includes),
    f"-D{ENGINEVER}",
    "-DREF_HARD_LINKED",
    "-DGAME_HARD_LINKED",
]

LDFlags = [LDFLAGS]

CFiles: list[str] = []

for file in os.listdir(f"{ENGINEVER.lower()}src/qcommon"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"{ENGINEVER.lower()}src/qcommon", file))
for file in os.listdir(f"{ENGINEVER.lower()}src/client"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"{ENGINEVER.lower()}src/client", file))
for file in os.listdir(f"{ENGINEVER.lower()}src/server"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"{ENGINEVER.lower()}src/server", file))
for file in os.listdir(f"game/{GAMENAME.lower()}"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"game/{GAMENAME.lower()}", file))
for file in os.listdir(f"sys/{SYS_BACKEND.lower()}"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"sys/{SYS_BACKEND.lower()}", file))
for file in os.listdir(f"{ENGINEVER.lower()}render/ref_{R_BACKEND.lower()}"):
    if file.endswith(".c"):
        CFiles.append(
            os.path.join(f"{ENGINEVER.lower()}render/ref_{R_BACKEND.lower()}", file)
        )

OFiles: list[str] = []

for file in CFiles:
    OFiles.append(file.replace(".c", ".o"))

print(f"CFlags: {CFlags}")


# compilation recipe
def compile_c(c_file):
    print(f"Compiling {c_file}")
    print(f"{CC} {' '.join(CFlags)} -c {c_file} -o {c_file.replace('.c', '.o')}")
    if (
        os.system(
            f"{CC} {' '.join(CFlags)} -c {c_file} -o {c_file.replace('.c', '.o')}"
        )
        != 0
    ):
        exit(1)


def link():
    if (
        os.system(
            f"{LD} {' '.join(OFiles)} -o {GAMENAME.lower()}-c9x {' '.join(LDFlags)} "
        )
        != 0
    ):
        exit(1)


parser = argparse.ArgumentParser(description="Build Script")
parser.add_argument("--clean", action="store_true", help="Clean the build directory")
parser.add_argument(
    "--rebuild", action="store_true", help="Clean the build directory but still build"
)

args = parser.parse_args()


def clean():
    for file in OFiles:
        if os.path.exists(file):
            os.remove(file)
    if os.path.exists(f"{GAMENAME.lower()}-c9x"):
        os.remove(f"{GAMENAME.lower()}-c9x")


if args.clean:
    clean()
    exit(0)
if args.rebuild:
    clean()

for file in CFiles:
    compile_c(file)

link()
