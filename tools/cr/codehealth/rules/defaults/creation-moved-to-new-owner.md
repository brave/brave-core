---
name: defaults/creation-moved-to-new-owner
rubric: defaults
severity: regression
---

**Rule.** Brave's disabling stubs, and the Brave code that looks objects up,
follow upstream when it moves an object's creation to a new owner.

**Why.** Brave's stub stays where the object used to be created, so a move
re-enables it. Stubbing the class itself in `chromium_src` survives such moves.
Moves also reorder creation: `TabModel` runs `AttachTabHelpers`, and so Brave's
tab helper constructors, before `TabFeatures::Init`, so an `X::FromWebContents`
on a moved class now gets null. Accessors that return an invalid value rather
than null fail the same way, less visibly.

**How to check.** The candidates are "Migrate X to TabFeatures / UnownedUserData
/ BrowserWindowFeatures" commits. Match each moved class name against Brave's
stubs, and grep Brave for `X::FromWebContents` and the class's accessors
(`git grep -n '<Class>::\(FromWebContents\|Get\)'`). Desktop and Android differ.

## Examples

- TabHelpers → TabFeatures re-enabled `FontPrewarmerTabHelper` and
  `NetErrorTabHelper`; both are now stubbed in their class.
- M157, 8103.1: `BraveShieldsTabHelper` observes the `ContentFaviconDriver`.
- M158: `SessionTabHelper::IdForTab()` in the Ads and Rewards tab helpers.
