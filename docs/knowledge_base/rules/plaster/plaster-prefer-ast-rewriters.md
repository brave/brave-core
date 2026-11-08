---
name: brave/plaster-prefer-ast-rewriters
description:
  A plaster substitution uses an AST rewriter (make_virtual, add_friend,
  drop_final and others) where one fits, rather than a regex.
rubric: design
severity: should_fix
---

**Rule.** When a plaster adds a `regex:` substitution for a transformation that
one of plaster's AST rewriters performs, such as making a method virtual
(`make_virtual`), befriending a class (`add_friend`), dropping `final`
(`drop_final`), inserting code at the top or end of a function body, or
extending an enum, ask for the rewriter instead. `tools/cr/plaster.py --help`
lists the rewriters, and `--help <rewriter>` documents one.

**Why.** An AST rewriter matches the code's syntax rather than its text, so it
does not depend on formatting, and it states what is being done, leaving how to
the tool.

## Examples

Bad:

```yaml
substitutions:
  - description: 'Let the Brave subclass reach private members.'
    regex:
      re_pattern: '( private:)'
      replace: '\1\n  friend class BraveMultiContentsView;'
```

Good:

```yaml
substitutions:
  - description: 'Let the Brave subclass reach private members.'
    add_friend:
      class_name: MultiContentsView
      friend_type: class BraveMultiContentsView
```
