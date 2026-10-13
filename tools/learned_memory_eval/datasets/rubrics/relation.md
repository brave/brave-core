# Rubric: relation

Rubric version: 1

The input is two memories about the same user: an old memory that is stored, and
a new memory. Give the relation, and the list of all relations that are
acceptable.

Relations:

- `different`: they are about different things, so both stay.
- `same`: they say the same thing in other words. One stays.
- `replace`: the new memory makes the old memory out of date. The new text takes
  the place of the old text.
- `merge`: the new memory adds to the old memory, and one sentence can hold
  both.
- `same_topic`: both are lines of the same ongoing topic or project (the
  pipeline keeps both).

Decide in this order:

```
 not the same subject ................................... different
 the same thing in other words .......................... same
 the new text says that the old fact ended .............. replace
   ("moved from", "new job at", "switched to", "no longer")
 both cannot be true at the same time, the new is later . replace
   (light mode vs dark mode, short vs detailed answers, first grade vs kindergarten)
 the new fact adds to the old fact ...................... merge
   (a second dog, one more language)
 both can be true, or it is not clear ................... different
   (two jobs: "Works as a nurse" vs "Works as a teacher")
```

1. A wrong replace deletes true information. A wrong "different" only gives one
   extra memory, and the user can delete it. So when it is not clear, choose
   `different`.
2. A conflict is not enough for a replace. Two facts that look like a conflict
   (two jobs) stay as two memories, unless the text says that one ended, or both
   cannot be true.
3. A temporary state (a trip, a stay with parents this month) does not replace a
   lasting fact (a home): `different`.
4. `acceptable` always has the relation itself. Add another relation only when
   it is also correct (a second dog: `merge`, and `different` is also fine).

## Solved examples

1. new "Moved to Berlin from San Francisco in September 2026", old "Lives in San
   Francisco" → replace; acceptable [replace].
2. new "Works as a nurse", old "Works as a teacher" → different; acceptable
   [different]. Two jobs can both be true.
3. new "Adopted a second dog", old "Has a dog" → merge; acceptable [merge,
   different].
