---
name: brave/no-sensitive-data-in-logs
description:
  'Logs never contain sensitive data such as sync seeds, private keys, tokens or
  credentials, at any log level.'
rubric: security
severity: blocking
---

**Rule.** NEVER write sensitive data to a log, at any level including `VLOG` and
`DVLOG`: sync seeds and codes, private keys and mnemonics, access tokens,
passwords and other credentials. Log that the operation happened, or failed,
without the value.

**Why.** Logs end up in bug reports, terminals and files other software can
read. Even verbose logging, enabled in debug builds and by users following
debugging instructions, can expose the secret.

## Examples

Bad:

```cpp
VLOG(1) << "Sync seed: " << sync_seed;
```

Good:

```cpp
VLOG(1) << "Sync seed set successfully";
```
