---
name: brave/plaster-anchors-wildcard-incidental-detail
description:
  A plaster match names the anchor that carries its intent (the target, function
  or switch variable) and wildcards surrounding content that upstream may change
  without affecting the rewrite.
rubric: correctness
severity: should_fix
---

**Rule.** A plaster's `re_pattern` MUST NOT spell out incidental detail it does
not change, such as the current contents of a list, the other conditions of an
`if`, or the statements around its anchor. Match the anchor that carries the
rewrite's intent, the GN target, function or switch variable being changed, and
let `.*?` (with `re_flags: [DOTALL]` where it spans lines) absorb the rest.

**Why.** Every incidental detail in the match is a reason for it to fail on an
upstream change unrelated to the rewrite's purpose.

## Examples

Bad:

```yaml
re_pattern:
  '(public_deps = \[ "//chrome/browser:browser_public_dependencies" \])'
```

Good:

```yaml
re_pattern: '(source_set\("web_applications"\).*?\bpublic_deps\s*=\s*\[.*?\])'
re_flags: [DOTALL]
replace: '\1\n  public_deps += brave_web_applications_deps'
```
