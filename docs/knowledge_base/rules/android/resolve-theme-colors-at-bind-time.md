---
name: brave/resolve-theme-colors-at-bind-time
description:
  A custom Preference or view resolves colors from theme attributes in
  onBindViewHolder() or its equivalent, never once in its constructor.
rubric: correctness
severity: should_fix
---

**Rule.** When a custom Preference or view resolves a color from a theme
attribute (`R.attr.*`, `MaterialColors.getColor()`, `SemanticColorUtils` and
similar), it MUST do so each time it binds, in `onBindViewHolder()` or the
equivalent bind step, and NEVER resolve it in the constructor and keep it in a
field.

**Why.** A color resolved at construction is the one of the theme at that
moment. When the user switches between light and dark themes without the view
being recreated, the stored color is stale; resolving it at bind time picks up
the current theme.

## Examples

Bad:

```java
public class MyPreference extends Preference {
    private final int mTextColor;

    public MyPreference(Context context) {
        super(context);
        mTextColor = resolveThemeColor(context, R.attr.textColor);
    }
}
```

Good:

```java
public class MyPreference extends Preference {
    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        int textColor = resolveThemeColor(getContext(), R.attr.textColor);
        ((TextView) holder.findViewById(R.id.title)).setTextColor(textColor);
    }
}
```
