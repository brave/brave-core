# Rubric: relevance

Rubric version: 2

The input is a list of memories about one user (a file, with ids), a new message
that the user sends to an AI assistant, and sometimes the previous message. The
assistant gets the memories that are relevant to the message. Give three groups
of memory ids for the message.

## The groups

- `use` (also called `must`): the assistant needs this memory to answer well.
  The answer would be worse, or wrong for this user, without it. Examples: the
  diet for a dinner request, the city for "is there a good gym near me?", the
  pet's name for a question about the pet, the units wish for a distance
  question, the tone wish for a joke.
- `maybe` (also called neutral): the memory is loosely related. A good answer
  does not need it, and it does no harm. Either choice is fine.
- `skip`: all other memories. Within them, the `forbidden` ones are tempting but
  have nothing to do with the message: they share a word or a topic area, or
  they are a style wish that does not fit. An assistant that used them would add
  noise or would show private data for no reason.

## Rules

1. Read the whole pool. A message can need two or three memories.
2. A short follow-up ("and for two people?", "what about the dog?") has a
   `previous_message`. Use the previous message to find the topic, then use the
   same rules.
3. A message in another language is judged by its meaning. The memories are in
   English.
4. A general style wish ("Prefers short answers", "Prefers bullet points",
   "Writes in a dry tone") is `maybe` for most messages. It is `use` only when
   the message clearly links to it: units and a distance or a weight, a tone
   wish and a joke or a message to write, a language wish and a text to write, a
   format wish and a request for a list or a summary.
5. A fragment that lost its subject ("Is three years old.") is `use` only when
   the message makes the subject clear, and `maybe` or `skip` otherwise.
6. A message about a general topic (maths, science, code in a language that the
   user does not use) can have no `use` memory at all.
7. A memory about the same topic as the message is not always needed: a question
   about dog food needs the dog, not the user's job.
8. A sensitive memory (health, money) is `use` only when the message is about
   that topic. Otherwise it is `skip`, and it can be `forbidden`.

9. `use` is strict. Choose `use` only when the answer would be worse or wrong
   for this user without the memory. When a memory only adds helpful context,
   choose `maybe`.
10. When the message already says the fact (it names the city, the place, the
    dog), a memory that repeats it is `maybe`, not `use`.
11. A memory about the same activity or place as the message is `maybe` unless
    the answer changes because of it.

## Solved examples

Pool: m1 "Is vegetarian and does not want meat recipes.", m2 "Has a beagle named
Luna.", m3 "Writes TypeScript with React.", m4 "Prefers short answers.", m5
"Moved to Berlin."

1. "Can you suggest a quick dinner for tonight?" → use [m1]; maybe [m4]; skip
   m2, m3, m5 (forbidden m2, m3).
2. "Explain how photosynthesis works." → use []; maybe [m4]; skip m1, m2, m3,
   m5.
3. previous "I want to cook for friends on Saturday." message "and what about
   dessert?" → use [m1]; maybe [m4].
4. "How far is it to walk to the Central Library?" with m6 "Lives near Elm
   Street.", m7 "Visits the Central Library every week.", m8 "Wants distances in
   miles." → use [m6, m8]; maybe [m7]. The walk needs the start point and the
   units. The weekly visits do not change the distance.
