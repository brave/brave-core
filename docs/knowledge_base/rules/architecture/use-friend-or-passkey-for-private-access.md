---
name: brave/use-friend-or-passkey-for-private-access
description:
  'Give tests or other classes access to private members with a friend
  declaration or a base::PassKey parameter, not by making the members public or
  protected.'
rubric: design
severity: should_fix
---

**Rule.** Do not move a member from `private:` to `public:` or `protected:` just
so that a test or another class can reach it. Declare that class a `friend`, or,
to open one method of a Brave class to one caller, give the method a
`base::PassKey<Caller>` parameter. The rule is about loosening access control
for convenience: NEVER flag normal inheritance, where a subclass calls inherited
protected methods or adds public methods that delegate to them, and NEVER flag
`*ForTesting()` methods, an accepted Chromium convention.

**Why.** A member made public for one caller is open to every caller, and its
use spreads; `friend` and `base::PassKey` keep the access limited to the class
that needs it.

## Examples

Bad:

```cpp
 public:
  void InternalMethod();  // was private, made public for tests
```

Good:

```cpp
 private:
  friend class BraveExternalProcessImporterBridge;
  void InternalMethod();
```

```cpp
 public:
  void InternalMethod(base::PassKey<BraveExternalProcessImporterBridge>);
```
