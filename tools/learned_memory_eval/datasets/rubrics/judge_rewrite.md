# Rubric: judge, rewrite faithfulness

Rubric version: 1

A memory feature turns sentences that a user wrote into short memories. A
local language model writes the memories. You judge one output.

The input has the `turn_date` (the date of the chat), the `sentences` that the
user wrote, and the `memories` that the model made from them. The model had
this task: write each memory as a short fact about the user in the third
person without a subject ("Lives in Berlin"); use only what the sentences
say; do not guess; do not add names, numbers or other details; write one
memory for each idea; make relative dates ("next month", "just") exact with the
turn date.

Give three flags. Judge the memories together, as one answer.

- `added_detail`: a memory states something that the sentences do not state: a
  name, a number, a place, a reason, a time or a claim. A date that is the
  correct result of a relative date and the turn date is NOT an added detail.
  A wrong date is an added detail.
- `lost_detail`: an idea or a detail that matters (a fact, a name, a number, a
  place, a negation) is in the sentences, but in none of the memories. Filler
  words, greetings and a question that is not a fact do not count.
- `wrong_subject`: a memory is about another person than the sentences say, or
  it changes who does what, or it turns the meaning around (a negation is
  lost, "wants to" becomes "does"). A memory about the user's dog is correct
  when the sentences say so.

The output is faithful when all three flags are false.

## Rules

1. Judge the meaning, not the style. A change of word order, a shorter text
   and a plural are fine.
2. A memory that is a correct paraphrase is fine, also in another language
   (English memories from sentences in another language).
3. A memory that splits one idea into two correct memories is fine.
4. Judge only what the input shows. Do not use outside knowledge about the
   user.

## Solved examples

1. turn_date 2026-09-20, sentences ["We are moving to Berlin next month."],
   memories ["Moves to Berlin in 2026-10"] → no flags (faithful). The month is
   correct.
2. sentences ["I have a dog named Luna, she is a 3 year old beagle."], memories
   ["Has a beagle named Luna"] → lost_detail (the age is lost).
3. sentences ["I drive a Honda Civic."], memories ["Lives in Berlin"] →
   added_detail, lost_detail.
