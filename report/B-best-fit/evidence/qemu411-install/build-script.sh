#!/usr/bin/env bash
set -euo pipefail
ROOT="$HOME/.local/opt/oslab-qemu-4.1.1"
ARCHIVE=/mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/qemu-4.1.1.tar.xz
EVIDENCE=/mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/qemu411-build
mkdir -p "$EVIDENCE"
exec > >(tee "$EVIDENCE/build.log") 2>&1
trap 'rc=$?; echo "$rc" > "$EVIDENCE/build.exitcode"' EXIT
test ! -e "$ROOT"
mkdir -p "$ROOT"
printf "Independent root: %s
" "$ROOT"
sha256sum "$ARCHIVE" /opt/qemu/bin/qemu-system-riscv64 | tee "$EVIDENCE/hashes-before.txt"
tar -xJf "$ARCHIVE" -C "$ROOT"
mkdir "$ROOT/build"
cd "$ROOT/build"
../qemu-4.1.1/configure --prefix="$ROOT/install" --target-list=riscv32-softmmu,riscv64-softmmu --python=/usr/bin/python3 --disable-werror
make -j4
make install
"$ROOT/install/bin/qemu-system-riscv64" --version
sha256sum /opt/qemu/bin/qemu-system-riscv64 | tee "$EVIDENCE/existing-qemu-after.txt"
printf "%s
" "$ROOT/install/bin/qemu-system-riscv64" > "$EVIDENCE/executable.txt"
