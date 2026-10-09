# Rubric: merge

Rubric version: 1

The input is two memories about the same user: an old memory and a new
memory that adds to it. An LLM merges them into one memory, in the style of
the rewrite: a short fact in the third person, without a subject, with only
what the two memories say.

The gold says what a good merge must have:

- `must`: the items that the merged text must keep from both memories, one
  item for each key detail. An item is a word, or a list of words of which one
  must match (["two", "second", "2"]). The match is a case-insensitive part of
  the text.
- `must_not`: words that the merged text must not have: a wrong addition, or a
  word that shows that a detail was changed. Use an empty list when there is
  no clear risk.
- `canonical`: a reference merged memory.

## Solved examples

1. old "Has a dog", new "We adopted a second dog." → must [["two", "second",
   "2"]], must_not [], canonical "Has two dogs".
2. old "Speaks French", new "Is learning Spanish" → must ["french",
   "spanish"], must_not [], canonical "Speaks French and is learning Spanish".
3. old "Lives in Berlin", new "Lives in the Kreuzberg district" → must
   ["kreuzberg"], must_not [], canonical "Lives in the Kreuzberg district of
   Berlin".
