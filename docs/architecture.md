# Architecture

```text
CLI parser -> HTTP transport -> response parser -> normalized Location model
                                                     -> output formatter
```

`main.c` ignores SIGPIPE, validates options, handles offline help/version before
curl initialization, performs one request, parses the body, selects output, and
cleans each acquired resource once. Errors use the named exit statuses and stderr.
The process stays in the initial C locale for deterministic numeric output.

`cli.c` uses `getopt_long()` and returns a `CliConfig`. Only one distinct mode is
allowed. Repeated identical modes are accepted. Arguments are validated before
help or version is displayed. Neither informational mode initializes curl.

`http_client.c` owns a geometrically grown response buffer capped at 1 MiB after
decompression. Its callback checks multiplication and addition bounds before
allocation. Buffer capacity includes a NUL terminator. Timeouts are centralized
in `http_client.h`: connect 5 seconds, total 10 seconds. HTTP 429 is identified
even if the body cannot be received. No redirect, retry, alternate provider, or
HTTP downgrade occurs. All curl setup and status operations are checked. TLS
peer and hostname verification are explicit, and only HTTPS is permitted.

`location.c` parses a bounded byte span with cJSON, rejects trailing content,
requires an object and boolean success, and checks optional types. Absent, null,
empty, and ASCII-space-only strings become missing values. Terminal controls,
embedded NUL, invalid UTF-8, non-finite numbers, and out-of-range coordinates are
rejected. At least one city, region, or country identifier is required, including
in coordinate mode. Unknown provider keys are ignored for forward compatibility.

Callers initialize a `Location` before parsing. Parsing clears the previous
value, builds an independent temporary value, and transfers ownership only on
success or provider rejection. `location_free()` handles partially initialized
values and can be called repeatedly. Provider rejection retains its message but
never reaches a normal formatter. Strings are exact-size allocations, never
borrowed from a deleted cJSON tree. Coordinate presence flags distinguish zero
from absent. The parser temporarily installs malloc/free-compatible cJSON hooks
to distinguish syntax errors from allocation failures. This internal CLI API is
single-threaded and is not safe for concurrent cJSON users or custom global hooks.

`output.c` receives the model and a `FILE *`, not transport data. Missing mode
requirements fail before output. JSON is constructed and serialized entirely by
cJSON before writing. All formatters flush and check stream errors. Missing full
fields are `Unknown`; JSON fields are null. Coordinates use six decimal places
in text and cJSON's numeric representation in JSON. Accuracy prose lives in help
and documentation so the requested data formats remain script-compatible.

Tests use GNU/Linux facilities only in test code: `open_memstream`, pipes, linker
wrapping, and `/dev/full`. HTTP tests replace libcurl entry points at link time,
assert TLS/protocol/timeout settings, inject transfer/status failures, and drive
the production receive callback at its exact size boundary and beyond. Allocation
wrappers test partial cleanup. `build/geome-fixture` links an alternate transport
from `tests/fixture_http.c`; the production binary has no fixture environment
variables or local-data bypass. Shell integration validates the real main/CLI
flow. Live schema validation is opt-in and performs only one request.

Adding another provider requires an explicit privacy and terms decision, a new
transport endpoint selection policy, and its own parser adapter producing the
same owned Location model. Version 1 deliberately has no automatic fallback.
