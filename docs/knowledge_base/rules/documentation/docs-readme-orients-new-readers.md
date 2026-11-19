---
name: brave/docs-readme-orients-new-readers
description:
  "A major directory's README.md orients new developers: its purpose, key files,
  how to use the main APIs, and who maintains it, starting from the simplest use
  case."
rubric: readability
severity: should_fix
---

**Rule.** Give each major directory a README.md that orients new developers. It
identifies the purpose of the directory, its key files and their roles, how to
use its main APIs, and who maintains the code (team or component). Start with
the simplest use case and build up to advanced usage.

**Why.** The README is where newcomers start; one that is missing or empty sends
them to read the code to learn what it is for.

## Examples

Bad:

```markdown
# Rewards

TODO
```

Good:

```markdown
# Brave Rewards Browser Integration

This directory contains the browser-layer integration for Brave Rewards.

## Key Files

- `rewards_service_impl.cc` - Main service implementation
- `rewards_service_factory.cc` - Profile-keyed factory

## Usage

Get the service via the factory: `RewardsServiceFactory::GetForProfile(profile)`
```
