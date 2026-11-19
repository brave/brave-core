---
name: brave/java-patch-when-bytecode-adapter-cannot-apply
description:
  An upstream Java class with two constructors is changed with a direct
  .java.patch, because a bytecode class adapter cannot patch it.
rubric: design
severity: should_fix
---

**Rule.** Do not write a bytecode class adapter (`changeSuperName()`,
`redirectConstructor()` and the like in `build/android/bytecode/`) for an
upstream class that has two constructors: change that class with a direct
`.java.patch` file instead. Conversely, a `.java.patch` for such a class is the
expected tool, not a missed adapter.

**Why.** Bytecode patching fails for a class with two constructors, so an
adapter for one either breaks the build or leaves the class unpatched; the
direct patch is the mechanism that works.

## Examples

Bad:

```java
// UpstreamClass has two constructors: UpstreamClass() and UpstreamClass(Context).
public BraveUpstreamClassClassAdapter(ClassVisitor visitor) {
    super(visitor);
    changeSuperName(sUpstreamClassName, sBraveUpstreamClassBaseName);
}
```

Good:

```diff
# patches/chrome-android-java-src-org-chromium-chrome-browser-UpstreamClass.java.patch
-public class UpstreamClass extends BaseClass {
+public class UpstreamClass extends BraveUpstreamClassBase {
```
