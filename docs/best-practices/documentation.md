# Documentation Best Practices

<a id="DOC-001"></a>

## ✅ Update Documentation Alongside Code Changes

**Documentation changes must accompany code modifications in the same changelist.** Don't create separate CLs for documentation updates — they tend to get forgotten or lose context. Updating docs in the same CL ensures accuracy and provides reviewers with context about the changes being made.

```
# ❌ WRONG - code change in one CL, docs "later"
CL 1: Refactor RewardsService API
CL 2: (never created) Update RewardsService docs

# ✅ CORRECT - docs updated in the same CL
CL 1: Refactor RewardsService API + update README and method docs
```

---

<a id="DOC-002"></a>

## ✅ Delete Dead Documentation

**Remove outdated documentation rather than letting it rot.** Obsolete docs mislead developers and erode trust in the codebase. When you encounter documentation that no longer matches the code, delete or update it.

- Remove content you're certain is incorrect
- Default toward removal when in doubt
- An absent doc is better than a wrong doc

```cpp
// ❌ WRONG - outdated comment left in place
// This method fetches data from the legacy XML endpoint.
void FetchData() {
  // Actually uses JSON now...
}

// ✅ CORRECT - comment updated or removed
// Fetches user data from the JSON API.
void FetchData() { ... }
```

---

<a id="DOC-004"></a>

## ✅ Method Documentation Should Describe the Contract

**Method API documentation should describe the contract: behavior, parameters, return values, gotchas, restrictions, and exceptions.** This is especially important for public methods in headers.

```cpp
// ❌ WRONG - vague or missing docs
// Gets the publisher info.
PublisherInfo GetPublisherInfo(const std::string& id);

// ✅ CORRECT - describes the contract
// Returns publisher info for |id|, or nullopt if not found in the
// local database. Does not trigger a network fetch. Must be called
// on the UI thread. The returned info may be stale if the publisher
// hasn't been visited recently.
std::optional<PublisherInfo> GetPublisherInfo(const std::string& id);
```

---

<a id="DOC-005"></a>

## ✅ README.md Files Should Orient New Readers

**Each major directory should have a README.md that orients new developers.** A good README identifies:

- The purpose of the directory
- Key files and their roles
- How to use the main APIs
- Who maintains the code (team or component)

Start with the simplest use case and build up to advanced usage.

```markdown
# ❌ WRONG - no README, or an empty one
(nothing)

# ✅ CORRECT - orienting README
# Brave Rewards Browser Integration

This directory contains the browser-layer integration for Brave Rewards.

<a id="DOC-006"></a>

## Key Files
- `rewards_service_impl.cc` - Main service implementation
- `rewards_service_factory.cc` - Profile-keyed factory

<a id="DOC-007"></a>

## Usage
Get the service via the factory:
RewardsServiceFactory::GetForProfile(profile)
```

---

<a id="DOC-008"></a>

## ✅ Don't Duplicate Documentation — Link to Canonical Sources

**Link to existing documentation rather than creating duplicates.** Duplicate docs inevitably drift out of sync. If you need project-specific context, add a brief note and link to the canonical source.

```markdown
# ❌ WRONG - duplicating upstream docs

<a id="DOC-009"></a>

## How to Add a Feature Flag
1. Create a feature in features.h...
2. Register in about_flags.cc...
(copy of upstream instructions that will go stale)

# ✅ CORRECT - link with context

<a id="DOC-010"></a>

## How to Add a Feature Flag
Follow the [Chromium feature flag guide](https://chromium.googlesource.com/chromium/src/+/main/docs/how_to_add_your_feature_flag.md).
Brave-specific: also register in `brave/browser/about_flags.cc`.
```

---

<a id="DOC-011"></a>

## ✅ Prefer Minimal, Fresh Documentation Over Comprehensive Stale Documentation

**A small amount of accurate, up-to-date documentation is more valuable than extensive documentation in poor condition.** Treat documentation like a bonsai tree — alive but frequently trimmed.

- Don't write docs you won't maintain
- Remove sections that nobody reads or updates
- Focus on the docs that save the most developer time
