# Privacy

## What Leaves the Machine

For a location command, libcurl makes one HTTPS GET to `https://ipwho.is/`.
There is no query parameter, explicit IP argument, request body, or API token.
Application headers include `User-Agent: geome/1.0.0` (the actual build version),
`Accept: application/json`, and supported content encodings. libcurl generates
normal HTTP host and protocol headers and TLS handshake metadata. A proxy
explicitly configured through libcurl's environment support may add its own
authentication or routing metadata; the application itself creates no credentials.

The provider necessarily receives the public IP used for the request. Your DNS
resolver may receive the provider hostname. Network intermediaries see IPs,
timing, and connection metadata. A configured proxy sees the connection; a TLS
interception proxy trusted by the system CA store may also see request content.
The provider's own logging, retention, subprocessors, and policy are governed by
https://ipwhois.io/privacy and are outside geome's control.

No SSIDs, MAC addresses, interface names, local addresses, network configuration,
files, or user-created content are collected. There is no telemetry. Help and
version perform no network access. The binary does not support a runtime endpoint
override, local fixture mode, API token, or result upload.

## What Stays

The response is held in bounded process memory, normalized, written to the
selected stdout format, then freed. geome creates no config, cache, log, or
temporary files and does not retain or send the result elsewhere. Provider error
messages can appear on stderr; full response bodies are not logged. A terminal,
redirection, caller, swap, or OS crash-dump policy may preserve output or memory;
the program neither controls those mechanisms nor promises secure memory erasure.

Proxy environment variables supported by libcurl include `https_proxy`,
`HTTPS_PROXY`, `all_proxy`, `ALL_PROXY`, `no_proxy`, and `NO_PROXY`; lowercase
variants take precedence. Using them can change both the recipient path and the
apparent location. No custom application environment variables are supported.
`GEOME_LIVE_TESTS` and `GEOME_TEST_*` belong only to test tools.

## Appropriate Use

This is approximate public IP geolocation, not GPS or proof of physical presence.
VPNs, proxies, Tor, corporate and mobile gateways, and ISP registration can point
to a different place. Never use these results for emergency response, safety
decisions, legal residency, authentication, or precise tracking. Review the
provider's current terms before deployment; software licensing grants no rights
to resell a provider's service or database.
