#!/usr/bin/env bash
# Fails if a test is marked RUN_TEST_KNOWN_BUG for an issue that isn't open.
# A closed issue means the bug is fixed (switch the test to RUN_TEST) or won't
# be fixed (delete the test). Needs gh with read access to the repo's issues.
set -euo pipefail

status=0
checked=0
declare -A state_of

while IFS=: read -r file line match; do
  issue=$(grep -oE '[0-9]+$' <<<"$match")
  if [[ -z "${state_of[$issue]:-}" ]]; then
    state_of[$issue]=$(gh issue view "$issue" --json state -q .state 2>/dev/null || echo MISSING)
  fi
  state=${state_of[$issue]}
  checked=$((checked + 1))
  if [[ "$state" != "OPEN" ]]; then
    echo "::error file=$file,line=$line::Known-bug test marks issue #$issue, which is $state. Switch the test to RUN_TEST if the bug is fixed."
    status=1
  fi
done < <(git ls-files 'src/*.c' | xargs grep -noE 'RUN_TEST_KNOWN_BUG(_WITH)?\(\s*\w+\s*,\s*[0-9]+')

echo "Checked $checked known-bug tests against ${#state_of[@]} issues."
exit $status
