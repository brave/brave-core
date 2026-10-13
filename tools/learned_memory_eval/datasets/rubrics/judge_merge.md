# Rubric: judge, merge faithfulness

Rubric version: 1

A memory feature keeps short memories about a user. When a new memory adds to an
old memory, a local language model writes one merged memory. You judge one
output.

The input has the `old` memory, the `new` memory and the `merged` memory. The
model had this task: write one memory that holds both; use only what the two
memories say; do not guess; do not add names, numbers or other details.

Give three flags.

- `added_detail`: the merged memory states something that neither input memory
  states: a name, a number, a place, a reason or a claim.
- `lost_detail`: a fact of the old or the new memory is missing in the merged
  memory. The new memory adds to the old one, so both facts must stay. A count
  that the new memory changes ("a dog" and "a second dog" → "two dogs") is not
  lost.
- `wrong_subject`: the merged memory is about another person than the inputs
  say, or it changes who does what, or it turns the meaning around.

The output is faithful when all three flags are false.

## Rules

1. Judge the meaning, not the style. A shorter or re-ordered text is fine.
2. A correct count that the two memories give together is fine ("Has a dog named
   Luna" and "Adopted a second dog named Pixel" → "Has two dogs, Luna and
   Pixel").
3. Judge only what the input shows.

## Solved examples

1. old "Has a dog named Luna.", new "Adopted a second dog named Pixel." merged
   "Has two dogs, Luna and Pixel." → no flags (faithful).
2. old "Speaks German.", new "Speaks Spanish too." merged "Speaks Spanish." →
   lost_detail.
3. old "Lives in Berlin.", new "Works at a bakery in Kreuzberg." merged "Lives
   in Berlin and works at a bakery in Kreuzberg since 2020." → added_detail.
