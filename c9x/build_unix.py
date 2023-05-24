import os
import sys

CC = "cc" if os.getenv("CC") is None else str(os.getenv("CC"))
LD = "cc" if os.getenv("LD") is None else str(os.getenv("LD"))
CFLAGS = "-O1 -g -w --std=c9x" if os.getenv("CFLAGS") is None else str(os.getenv("CFLAGS"))
LDFLAGS = "-lm -lSDL2 -g" if os.getenv("LDFLAGS") is None else str(os.getenv("LDFLAGS"))

Includes = ["-I."]

if os.getenv("INCLUDES") is not None:
    Includes.append(str(os.getenv("INCLUDES")))

GAMENAME = "QUAKE" if os.getenv("GAMENAME") is None else str(os.getenv("GAMENAME"))

SYS_BACKEND = "SDL" if os.getenv("SYS_BACKEND") is None else str(os.getenv("SYS_BACKEND"))

CFlags = [CFLAGS, f"-DQ_GAMENAME=\"{GAMENAME}\"", ' '.join(Includes)]

LDFlags = [LDFLAGS]

CFiles : list[str] = []

for file in os.listdir("."):
    if file.endswith(".c"):
        CFiles.append(os.path.join("", file))
for file in os.listdir(f"game/{GAMENAME.lower()}"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"game/{GAMENAME.lower()}", file))
for file in os.listdir(f"sys/{SYS_BACKEND.lower()}"):
    if file.endswith(".c"):
        CFiles.append(os.path.join(f"sys/{SYS_BACKEND.lower()}", file))

OFiles : list[str] = []

for file in CFiles:
    OFiles.append(file.replace(".c", ".o"))

print(f"CFlags: {CFlags}")
# compilation recipe
def compile_c(c_file):

    print(f"Compiling {c_file}")
    print(f"{CC} {' '.join(CFlags)} -c {c_file} -o {c_file.replace('.c', '.o')}")
    if os.system(f"{CC} {' '.join(CFlags)} -c {c_file} -o {c_file.replace('.c', '.o')}") != 0:
        exit(1)

def link():
    if os.system(f"{LD} {' '.join(OFiles)} -o {GAMENAME.lower()}-c9x {' '.join(LDFlags)} ") != 0:
        exit(1)

for file in CFiles:
    compile_c(file)

link()
