# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 1.x     | ✓         |

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities.

Email: security@opensynaptic.org (or open a private security advisory on GitHub).

We aim to acknowledge all reports within 72 hours and publish a fix within 14 days.

## Scope

This library runs on MCUs and processes received wire packets. The main attack
surface is a **malformed or malicious packet**. The library validates:

- Minimum frame length (16 bytes)
- Both CRC fields before exposing body data
- Body segment lengths against compile-time limits

Callbacks are only invoked after structural validation. All buffers are
statically sized (no heap allocation).
