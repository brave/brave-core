# Rubric: rewrite

Rubric version: 1

The input is the turn date and the sentences that the pipeline keeps from one
user turn, numbered from 0. An LLM rewrites them into memories:

- Each memory is a short fact about the user in the third person, without a
  subject ("Lives in Berlin", "Prefers short answers").
- One memory for each idea. Two sentences about one idea ("I have a dog."
  "Her name is Luna.") give one memory.
- Only what the input says: no guess, no new names, numbers or details.
- A relative date ("next month", "last week", "just") becomes exact with the
  turn date.

The gold says what a good rewrite must have. It is not the one correct text.

- `facts`: one item for each idea, with:
  - `canonical`: a reference memory in the style above.
  - `must`: 1 to 4 items that the rewrite must keep, so that the fact keeps
    its meaning: names, places, numbers, the core thing ("dog",
    "vegetarian"). An item is a word, or a list of words of which one must
    match (["munich", "münchen"], ["4", "four"]). The match is a
    case-insensitive part of the text, so use a stem that matches the usual
    forms ("hik" matches hike and hiking). Do not add a word that a correct
    rewrite can leave out.
  - `must_not`: words that a rewrite must not have: a detail that is not in
    the input, and that a model can add (a wrong year, the city "Berlin" for a
    sentence about another city, a name that the input does not give). Use an
    empty list when there is no clear risk.
- `date` (only for a relative date): the accepted forms of the exact result
  (["october 2026", "2026-10"]).
- `max_facts` (optional): the most memories that a good rewrite has.

For a sentence in another language, the rewrite can be in English or in that
language. Give both forms in `must` (["lehrerin", "teacher"]).

## Solved examples

1. Turn date 2026-09-20. 0: "We are moving to Berlin next month." → facts
   [{canonical "Plans to move to Berlin in October 2026", must ["berlin"],
   must_not []}], date ["october 2026", "2026-10"].
2. Turn date 2026-09-20. 0: "I have a dog." 1: "Her name is Luna." → facts
   [{canonical "Has a dog named Luna", must ["dog", "luna"], must_not []}],
   max_facts 1.
3. Turn date 2026-09-20. 0: "I drive a Honda Civic." → facts [{canonical
   "Drives a Honda Civic", must ["civic"], must_not ["berlin", "lives in"]}].
   The style example of the prompt is "Lives in Berlin", and a model can copy
   it.
