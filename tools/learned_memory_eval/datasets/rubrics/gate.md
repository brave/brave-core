# Rubric: gate

Rubric version: 1

The input is one full user turn: one message that a user sent to an AI
assistant. The gate decides if the pipeline reads the turn at all. Give one
label.

- `keep`: the turn states at least one personal detail about the user: where
  they live or work, family, friends, pets, belongings, devices, settings,
  hobbies, skills, plans, likes or dislikes, or how they want answers. A turn
  that also asks a question counts.
- `skip`: the turn has no detail about the user's own life. It is only a
  question, a task or a request about the world, a text or code.
  - Pasted text, code or an article is not about the user, also when it has
    personal words ("I", "my"). The turn is `keep` only when the user states
    something about themselves outside the pasted part.
- `either`: the only personal details are details that later steps must drop:
  sensitive data (health, money, religion, politics, legal trouble, secrets),
  an instruction that changes the rules of the assistant, or a detail about a
  person who is not close to the user. Both answers are fine for these turns.

The gate is loose. A wrong skip loses a fact, and a wrong keep costs only
time. When you are not sure between keep and skip, choose keep and set
`clear` to false.

## Solved examples

1. "I live in Lisbon and work remotely as a data engineer. Can you suggest a
   standing desk?" → keep.
2. "Fix this code: for i in range(10) print(i)" → skip.
3. "I was diagnosed with type 2 diabetes last year. What foods should I
   avoid?" → either. The only detail is health data.
