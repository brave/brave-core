---
name: defaults/new-enum-values-and-id-headroom
rubric: defaults
severity: regression
---

**Rule.** New upstream enum values reach the right case in Brave's switches and
filters, and Brave's ID ranges keep their headroom.

**Why.** A switch sends a new value to its `default:` case, and Brave code that
iterates an upstream list with its own filter gets the new value too. Command
IDs and syncable pref IDs collide when upstream grows into Brave's range.

**How to check.** The candidates add enum values in headers Brave includes, or
IDs in `chrome/app/chrome_command_ids.h` and the syncable pref lists. Grep
Brave's switches and filters over each enum, and compare a filter with
upstream's new condition.

## Examples

- M157, 8097.1: `BrowsingDataType.SKILLS` reached Brave's "Clear on exit" page,
  which has no pref for it.
