# Plaster

<!-- See also: patches.md for general patch best practices -->

<a id="PLSTR-001"></a>

## ✅ Plaster Patch Patterns Should Match Specific Context

**Plaster patch config `re_pattern` should generally match method names or other
relevant context to ensure a single, targeted match.** Simple patterns can be
used when the intention is to match all instances of a particular pattern in a
file.

```yaml
# ❌ WRONG - overly broad pattern that might match multiple locations
re_pattern: 'return false;'

# ✅ CORRECT - matches method name and context for targeted replacement
re_pattern: 'bool IsFeatureEnabled[\(\)\S\s\{\}]+?(return false);\s+^\}'

# ✅ ALSO CORRECT - simple pattern when all instances should match
# (Use this intentionally when you want to replace every occurrence)
re_pattern: 'kOldConstant'
```

Matching specific context (method names, surrounding code) makes patches more
maintainable and prevents accidental matches during Chromium updates. Use broad
patterns only when you explicitly intend to replace all occurrences.

---

<a id="PLSTR-002"></a>

## ✅ Use `pattern` for Simple Symbol Replacement, `re_pattern` for Context-Aware Matches

**Only use `pattern` for simple matches—generally a single symbol name that you
want to replace globally.** If you need more context (including whitespace),
then `re_pattern` is more appropriate.

```yaml
# ✅ CORRECT - simple symbol replacement with pattern for all instances of a constant
pattern: 'kOldConstantName'
replace: 'kNewConstantName'

# ✅ CORRECT - simple symbol replacement with pattern for all instances of a method call
pattern: 'ChromiumMethod'
replace: 'BraveMethod'

# ✅ CORRECT - re_pattern handles flexible whitespace
re_pattern: '(^\s+)(ChromiumMethod\(\))'
replace: '\1BraveMethod()'

# ❌ WRONG - pattern requires exact whitespace match, fragile to upstream changes
pattern: '    MyMethod()'  # Breaks if upstream changes indentation
replace: '    BraveMethod()'

# ✅ CORRECT - re_pattern handles flexible whitespace
re_pattern: '(if\s+\()MyMethod\(\)([\S\s]+?{)'
replace: '\1BraveMethod()\2'

# ❌ WRONG - pattern includes additional context
pattern: 'if (MyMethod() && my_bool) {'  # Breaks if upstream changes indentation
replace: 'if (BraveMethod() && my_bool) {'
```

Using `pattern` for simple symbol names keeps configs readable and maintainable.
Reserve `re_pattern` for when you need regex features like whitespace matching,
character classes, or structural patterns.

---

<a id="PLSTR-003"></a>

## ✅ Use Plaster to change private fields/methods visibility in Java for new code

````yaml
# ✅ CORRECT - simple visibility replacement for a field
pattern: 'private final @Nullable View mNtpHeader;'
replace: 'protected final @Nullable View mNtpHeader;'

# ❌ WRONG - Bytecode manupulation
```Java

        deleteField(sBraveFeedSurfaceCoordinatorClassName, "mNtpHeader");
        makeProtectedField(sFeedSurfaceCoordinatorClassName, "mNtpHeader");
````

---
