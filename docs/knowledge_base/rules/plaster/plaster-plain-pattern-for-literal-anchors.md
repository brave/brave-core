---
name: brave/plaster-plain-pattern-for-literal-anchors
description:
  When a plaster's anchor is a single literal statement, it uses pattern, not a
  re_pattern with escaped punctuation and capture groups that reproduce
  whitespace.
rubric: readability
severity: should_fix
---

**Rule.** When the text a plaster replaces is one literal statement, every token
of which changes, write it as `pattern:`. NEVER write it as a `re_pattern` that
escapes its punctuation (`\(`, `\)`) and captures leading whitespace only to put
it back. Keep `re_pattern` for matches that need regex features: flexible
whitespace, character classes or structural anchors.

**Why.** `pattern` escapes the string itself, so there is nothing to get wrong,
and the literal reads as the code it matches. The regex form adds escaping and
capture groups that can be wrong and say nothing more.

## Examples

Bad:

```yaml
re_pattern: '\n([^\S\n]*)return NavigateWebAppUsingParams\(nav_params\);'
replace: |-
  \n\1return BraveNavigateWebAppUsingParams(&profile_.get(), *params_, nav_params);
```

Good:

```yaml
pattern: 'return NavigateWebAppUsingParams(nav_params);'
replace:
  'return BraveNavigateWebAppUsingParams(&profile_.get(), *params_, nav_params);'
```
