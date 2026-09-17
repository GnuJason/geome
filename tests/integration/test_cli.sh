#!/bin/sh
set -eu
program=${GEOME_TEST_PROGRAM:-./geome}
fixture_program=./build/geome-fixture
scratch=$(mktemp -d ./build/cli.XXXXXX)
trap 'rm -rf "$scratch"' EXIT HUP INT TERM
export GEOME_TEST_FIXTURE=tests/fixtures/success.json

run_case() {
    expected=$1
    shift
    actual=0
    "$@" >"$scratch/out" 2>"$scratch/err" || actual=$?
    test "$actual" -eq "$expected" || {
        printf 'expected exit %s, got %s: %s\n' "$expected" "$actual" "$*" >&2
        cat "$scratch/err" >&2
        exit 1
    }
    if test "$expected" -eq 0; then
        test ! -s "$scratch/err"
    else
        test ! -s "$scratch/out"
        grep -q '^geome: ' "$scratch/err"
    fi
}

for option in --help -h --version -V; do
    run_case 0 "$program" "$option"
done
run_case 0 "$program" --version
printf 'geome %s\n' "${GEOME_TEST_VERSION:-1.0.0}" >"$scratch/expected"
cmp "$scratch/expected" "$scratch/out"
for option in --unknown -x --city=value; do
    run_case 2 "$program" "$option"
    grep -q -- '--help' "$scratch/err"
done
run_case 2 "$program" positional
run_case 2 "$program" -- positional
run_case 2 "$program" --help --unknown
run_case 2 "$program" --city --json
run_case 2 "$program" --coords --full
run_case 2 "$program" -cf
run_case 0 "$fixture_program"
printf 'Charlotte, NC, US\n' >"$scratch/expected"
cmp "$scratch/expected" "$scratch/out"
for option in --city -c; do
    run_case 0 "$fixture_program" "$option"
    printf 'Charlotte\n' >"$scratch/expected"
    cmp "$scratch/expected" "$scratch/out"
done
run_case 0 "$fixture_program" -c --city
run_case 0 "$fixture_program" --coords
printf '35.227100,-80.843100\n' >"$scratch/expected"
cmp "$scratch/expected" "$scratch/out"
for option in --json -j; do
    run_case 0 "$fixture_program" "$option"
    ./build/validate_json <"$scratch/out"
done
for option in --full -f; do
    run_case 0 "$fixture_program" "$option"
    printf '%s\n' 'City: Charlotte' 'Region: North Carolina' 'Region code: NC' \
        'Country: United States' 'Country code: US' 'Latitude: 35.227100' \
        'Longitude: -80.843100' 'IP: 198.51.100.10' 'ISP: Example ISP' >"$scratch/expected"
    cmp "$scratch/expected" "$scratch/out"
done
export GEOME_TEST_FIXTURE=tests/fixtures/missing_city.json
run_case 5 "$fixture_program" --city
grep -q 'try' "$scratch/err"
export GEOME_TEST_FIXTURE=tests/fixtures/missing_optional_fields.json
run_case 5 "$fixture_program" --coords
run_case 0 "$fixture_program" --json
./build/validate_json <"$scratch/out"
export GEOME_TEST_FIXTURE=tests/fixtures/api_failure.json
run_case 4 "$fixture_program"
grep -q 'Reserved IP address' "$scratch/err"
export GEOME_TEST_FIXTURE=tests/fixtures/malformed.json
run_case 5 "$fixture_program"
for status in 3 4 5 6; do
    export GEOME_TEST_FAILURE=$status
    run_case "$status" "$fixture_program"
done
unset GEOME_TEST_FAILURE
status=0
"$program" --help >/dev/full 2>"$scratch/err" || status=$?
test "$status" -eq 7
grep -q '^geome: could not write output' "$scratch/err"
if test "${GEOME_LIVE_TESTS:-0}" = 1; then
    run_case 0 "$program" --json
    ./build/validate_json <"$scratch/out"
    printf 'Live schema check passed (one request; location not logged).\n'
fi
printf 'CLI integration tests passed\n'
