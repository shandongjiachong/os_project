#!/usr/bin/env bash
set -u -o pipefail
ROOT=/mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023
EVID="$ROOT/submission/report/B-best-fit/evidence/final-20261024"
mkdir -p "$EVID"
cd "$ROOT/submission/code" || exit 1
GCCPREFIX=/opt/riscv-elf-toolchains/bin/riscv64-unknown-elf-
QEMU=/home/ubuntu/.local/opt/oslab-qemu-4.1.1/install/bin/qemu-system-riscv64
name="$1"
case "$name" in
clean) set -- make clean ;;
build) set -- make GCCPREFIX="$GCCPREFIX" -j2 ;;
qemu) set -- timeout --signal=TERM --kill-after=5s 15s make GCCPREFIX="$GCCPREFIX" QEMU="$QEMU" qemu ;;
grade) GRADE_TMP=$(mktemp -d /tmp/lab2_B_2412224_capture_XXXXXXXX); printf '%s\n' "$GRADE_TMP" > "$EVID/grade-temp-path.txt"; set -- timeout --signal=TERM --kill-after=5s 120s make GCCPREFIX="$GCCPREFIX" QEMU="$QEMU" GRADE_TMP="$GRADE_TMP" GRADE_QEMU_OUT="/tmp/lab2_B_2412224_final_serial.log" grade ;;
*) exit 3 ;;
esac
printf -v cmd '%q ' "$@"
printf 'START '; date --iso-8601=seconds
printf 'COMMAND %s\n' "$cmd" | tee "$EVID/$name.command.log"
script -q -e -c "$cmd" /dev/null 2>&1 | tee "$EVID/$name.log"
rc=${PIPESTATUS[0]}
if [ "$name" = grade ] && [ -f /tmp/lab2_B_2412224_final_serial.log ]; then
  cp /tmp/lab2_B_2412224_final_serial.log "$EVID/grade-serial.log"
fi
printf '%s\n' "$rc" > "$EVID/$name.exitcode"
printf 'ACTUAL EXIT %s = %s\n' "$name" "$rc"
printf 'END '; date --iso-8601=seconds
exit "$rc"
