---
name: brave/plaster-pattern-only-for-replaced-text
description:
  A plaster literal pattern holds only text being replaced, with no leading
  indentation and no adjacent tokens that do not change; matches needing more
  context use re_pattern.
rubric: correctness
severity: should_fix
---

**Rule.** A literal `pattern:` in a plaster MUST consist only of text that the
substitution replaces: a symbol name, or a whole statement every token of which
changes. NEVER put leading indentation in it, or neighbouring tokens that stay
the same (the other operands of a condition, a trailing brace). When the match
has to reach beyond the replaced text, use `re_pattern` with flexible whitespace
and capture groups instead.

**Why.** A literal containing text that is not being changed breaks when
upstream re-indents or edits that text, for a reason unrelated to the rewrite.

## Examples

Bad:

```yaml
pattern: 'if (MyMethod() && my_bool) {'
replace: 'if (BraveMethod() && my_bool) {'
```

Good:

```yaml
re_pattern: '(if\s+\()MyMethod\(\)([\S\s]+?{)'
replace: '\1BraveMethod()\2'
```
