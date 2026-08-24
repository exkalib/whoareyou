#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="$ROOT/Source/WorldSimDemo"

fail() {
    printf 'CONTRACT ERROR: %s\n' "$1" >&2
    exit 1
}

command -v rg >/dev/null 2>&1 || fail "ripgrep (rg) is required"

while IFS= read -r header; do
    if rg -q '^#include ".*\.generated\.h"$' "$header"; then
        last_include="$(rg '^#include ' "$header" | tail -1)"
        [[ "$last_include" == *'.generated.h"' ]] \
            || fail "$header does not keep its generated header as the final include"
    fi
done < <(find "$SOURCE" -maxdepth 1 -name '*.h' -type f | sort)

association="$(rg -o '"EngineAssociation"[[:space:]]*:[[:space:]]*"[^"]+"' "$ROOT/WorldSimDemo.uproject")"
[[ "$association" == *'"5.8"' ]] || fail "WorldSimDemo.uproject is not associated with UE 5.8"

for target in "$ROOT/Source/WorldSimDemo.Target.cs" "$ROOT/Source/WorldSimDemoEditor.Target.cs"; do
    rg -q 'DefaultBuildSettings[[:space:]]*=[[:space:]]*BuildSettingsVersion\.Latest' "$target" \
        || fail "$target does not use BuildSettingsVersion.Latest"
    rg -q 'IncludeOrderVersion[[:space:]]*=[[:space:]]*EngineIncludeOrderVersion\.Latest' "$target" \
        || fail "$target does not use EngineIncludeOrderVersion.Latest"
done

if rg -n 'ClaimedOpportunity\.Title|Event\.Time\b' "$SOURCE"; then
    fail "legacy invalid field reference found"
fi

if rg -n 'GameInstanceSubsystem' "$SOURCE"; then
    fail "world simulation source introduced a GameInstanceSubsystem"
fi

check_api() {
    local header="$1"
    local implementation="$2"
    local api="$3"
    rg -q "${api}\\(" "$header" || fail "$api declaration missing from $header"
    rg -q "::${api}\\(" "$implementation" || fail "$api implementation missing from $implementation"
}

check_api "$SOURCE/TruthLedgerSubsystem.h" "$SOURCE/TruthLedgerSubsystem.cpp" "QueryEvents"
check_api "$SOURCE/TruthLedgerSubsystem.h" "$SOURCE/TruthLedgerSubsystem.cpp" "GetRecentEvents"
check_api "$SOURCE/TruthLedgerSubsystem.h" "$SOURCE/TruthLedgerSubsystem.cpp" "TryGetEvent"
check_api "$SOURCE/PersonSubsystem.h" "$SOURCE/PersonSubsystem.cpp" "GetPeople"
check_api "$SOURCE/WorldSimulationSubsystem.h" "$SOURCE/WorldSimulationSubsystem.cpp" "GetActiveActivities"
check_api "$SOURCE/CommitmentSubsystem.h" "$SOURCE/CommitmentSubsystem.cpp" "GetActiveCommitments"
check_api "$SOURCE/OpportunityCompilerSubsystem.h" "$SOURCE/OpportunityCompilerSubsystem.cpp" "GetAvailableOpportunities"
check_api "$SOURCE/KnowledgeSubsystem.h" "$SOURCE/KnowledgeSubsystem.cpp" "GetKnowledgeRecords"
check_api "$SOURCE/WorldSimDebugSubsystem.h" "$SOURCE/WorldSimDebugSubsystem.cpp" "BuildSnapshot"
check_api "$SOURCE/WorldSimDemoBootstrapSubsystem.h" "$SOURCE/WorldSimDemoBootstrapSubsystem.cpp" "CreateDemoWorld"

self_test_file="$(mktemp)"
trap 'rm -f "$self_test_file"' EXIT
printf 'ClaimedOpportunity.Title\n' > "$self_test_file"
rg -q 'ClaimedOpportunity\.Title|Event\.Time\b' "$self_test_file" \
    || fail "forbidden-field detector self-test failed"

printf 'Static source contracts passed. This is not an Unreal Engine compile.\n'
