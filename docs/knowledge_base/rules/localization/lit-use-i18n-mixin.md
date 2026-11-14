---
name: brave/lit-use-i18n-mixin
description:
  Lit-based WebUI components get localized strings through I18nMixinLit, not by
  calling loadTimeData.getString() directly.
rubric: design
severity: should_fix
---

**Rule.** In a Lit-based WebUI component (a `CrLitElement`), apply
`I18nMixinLit` and use its `i18n()` methods. Do not call
`loadTimeData.getString()` directly from the component.

**Why.** The mixin is the standard approach and gives consistent i18n patterns
across components.

## Examples

Bad:

```ts
export class FooElement extends CrLitElement {
  protected get title_() {
    return loadTimeData.getString('fooTitle')
  }
}
```

Good:

```ts
const FooElementBase = I18nMixinLit(CrLitElement)

export class FooElement extends FooElementBase {
  protected get title_() {
    return this.i18n('fooTitle')
  }
}
```
