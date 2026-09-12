#!/usr/bin/env bash
# uic < 6.7 does not recognise the scoped enum names Designer >= 6.7 writes into
# .ui files, and silently swaps spacer size-policy axes. See HACKING.md.
#
#   check-ui-enums.sh        # report offenders, exit 1 if any
#   check-ui-enums.sh --fix  # rewrite them in place
set -euo pipefail

cd "$(dirname "$0")/.."

mapfile -t offenders < <(
    grep -rlE '<(enum|set)>[^<]*[A-Za-z_]+::[A-Za-z_]+::' --include='*.ui' src/ 2>/dev/null || true
)

if [[ ${#offenders[@]} -eq 0 ]]; then
    echo "check-ui-enums: OK -- no scoped enums in .ui files"
    exit 0
fi

if [[ "${1:-}" == "--fix" ]]; then
    printf '%s\0' "${offenders[@]}" | xargs -0 sed -i -E \
        '/<(enum|set)>/ s/\b([A-Za-z_][A-Za-z0-9_]*)::([A-Za-z_][A-Za-z0-9_]*)::([A-Za-z_][A-Za-z0-9_]*)\b/\1::\3/g'
    echo "check-ui-enums: normalized ${#offenders[@]} file(s):"
    printf '  %s\n' "${offenders[@]}"
    exit 0
fi

echo "check-ui-enums: ERROR -- fully-scoped enum names found in .ui files." >&2
echo "These break layout under Qt < 6.7 (silently, at build time)." >&2
echo >&2
for f in "${offenders[@]}"; do
    grep -nE '<(enum|set)>[^<]*[A-Za-z_]+::[A-Za-z_]+::' "$f" | sed "s|^|  $f:|" >&2
done
echo >&2
echo "Run 'scripts/check-ui-enums.sh --fix' to normalize them." >&2
exit 1
