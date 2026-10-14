---
name: webui/polymer-overrides-on-lit-elements
rubric: webui
severity: regression
---

**Rule.** Every tag Brave registers with `polymer_overriding` names an upstream
element that is still Polymer at the new tag.

**Why.** `polymer_overriding` only hooks Polymer elements, so an override of an
element that went Lit silently does nothing. Elements go Lit right after the
rebase fixed Brave's override of them, and earlier lifts left some dead, so
every tag needs checking every lift, not just the ones the range touched.

**How to check.** The candidate list is the output of
`tools/cr/codehealth/scripts/polymer_tags.py` for the range: each
`Register*('tag'` call and map key in Brave's `polymer_overriding` importers,
the upstream file whose `is` returns the tag, and its base class at both tags.
File an item per `broke` row, and per `dead` row not tracked yet. A `review` row
whose tag is found at neither tag is usually a rename: find the new element
(`git log -S"'<tag>'" -- chrome/browser/resources` in `src/`), which is dead for
Brave too. The second list, of the range's Polymer-to-Lit migrations, names the
elements Brave's other WebUI rules should look at.

## Examples

- M157, 8093.0: `settings-privacy-page`, `category-reference-card`.
- M157, 8094.1: `settings-ui`, `settings-menu` and the autofill page, right
  after the rebase had fixed the last two.
- M158, 8109.1: `settings-main`, whose `br/` override kept registering Polymer
  modifications.
- Payments and contact-info pages: overrides of Lit elements that never applied.
- Left dead by earlier lifts: sync controls, the performance page, the bookmarks
  side panel.
