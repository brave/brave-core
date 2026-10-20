# AI Chat CDP tools

Scripts that drive the dev build through its UI over the Chrome DevTools
Protocol (CDP), on test profiles that stay between runs. They test what unit
tests cannot: the real embedder, Leo's models, the local learned memory models,
and the pages.

Python 3.9 or newer, no packages.

| Script                   | Does                                                                                                 |
| ------------------------ | ---------------------------------------------------------------------------------------------------- |
| `search_profile.py`      | A profile with single-topic chats and memories for the semantic search of conversations and memories |
| `learned_memory_eval.py` | Learned memory (Dreaming) end to end: learn, review, modify, recall, delete                          |
| `demo_profile.py`        | The demo profile: conversation search, learning from chats, and one new memory with its trace        |
| `open_profile.py`        | Opens a test profile with `pnpm start`, to try it by hand                                            |
| `cdp.py`, `leo.py`       | The CDP client and the Leo helpers                                                                   |

## Profiles

A profile lives where `pnpm start --user_data_dir_name=<name>` puts it, so it is
outside the repo and outside `/tmp`, and a restart does not wipe it:

| Name                      | Path on macOS                                                         |
| ------------------------- | --------------------------------------------------------------------- |
| `leo-search-test`         | `~/Library/Application Support/BraveSoftware/leo-search-test`         |
| `leo-learned-memory-test` | `~/Library/Application Support/BraveSoftware/leo-learned-memory-test` |
| `leo-demo`                | `~/Library/Application Support/BraveSoftware/leo-demo`                |

On Linux the folder is `~/.config/BraveSoftware`. The state of the scripts, the
reports and the browser logs are in the `ai_chat_cdp` folder of the profile, so
deleting the profile deletes them too. `--profile <name>` picks another profile.

The scripts copy the embedder model (`BraveLocalAIModels`) from the profile of a
plain `pnpm start`. Start the dev build once with Settings > Privacy > Semantic
history search on, and wait until that folder exists.

## Before a run

- **Build first.** The scripts stop when a file outside `tools/` changed since
  the browser was built: a test of a binary without that code says nothing.
- **No other browser on the profile.** Quit it first; the scripts start and quit
  their own.

## Semantic search profile

```sh
./search_profile.py setup    # flag and settings on, embedder copied
./search_profile.py seed     # 7 chats and 5 memories, about 10 minutes
./search_profile.py verify   # runs the searches, prints what they find
./open_profile.py search     # try it by hand
```

Leo's default model answers the chats. `seed` resumes where it stopped;
`seed japan_trip` types one chat again.

## Learned memory eval

Needs Ollama on `localhost:11434` with `clef-flash:9b` and `qwen3.5:9b` (see
`../learned_memory_eval/README.md`). The eval loads both models first.

```sh
./learned_memory_eval.py --fresh            # new profile, about 15 minutes
./learned_memory_eval.py                    # resume after a failure
./learned_memory_eval.py --only learn       # one step again
./learned_memory_eval.py --set relations    # another chat set
./open_profile.py learned-memory            # try it by hand
```

The report is `ai_chat_cdp/runs/<time>/report.md` in the profile. To see what
each run and chat turn asked the local models, open
`brave://leo-memory-internals` in the profile.

Dreaming also runs by itself 1 minute after start. Its review waits in memory,
and the eval reviews a waiting one before it selects "Learn from chats".

## Demo profile

`demo/leo_demo.json` is Maya, an ICU nurse in Barcelona, over four months: 15
chats on many topics, some with several turns that each cover something else.
Seven facts are in her messages (her job, home, husband Tomas, daughter Ines,
Maine Coon cat Mochi, pescatarian diet, cello), and one preference for the
answers (short, in bullet points). Five things must not be learned: a card
number, a medicine, a child's fever, an order to always answer in Catalan, and a
week of night shifts. The preference shows the two questions of a chat turn: a
preference goes when the answer must follow it, and a fact when it is relevant
to the message.

```sh
./demo_profile.py build      # the profile, about 2 minutes
./demo_profile.py verify     # each demo search opens its chat at its turn
./demo_profile.py rehearse   # the demo on a copy, about 5 minutes
./open_profile.py demo       # open it for the demo
./demo_profile.py reset      # after a demo, back to the built state
```

The chats come in through the eval mode of the browser, which writes them with
their dates. Its Dreaming run is removed afterwards, so the profile has no
learned memory, and the daily run is a day away.

The demo chats use the local `qwen3.5:9b` of Ollama (`demo_profile.CHAT_MODEL`),
the same model as the Dreaming LLM, with its tool support on.

The demo browser starts with `certain_threshold` 0.75 (`demo_profile.SWITCHES`;
the feature default is 0.8). With 0.8 the decision model gives 0.79 for "merge"
in step 4, it is not certain, and the kitten becomes a second memory instead of
a change of the memory about Mochi.

Demo steps, in brave://leo-ai:

1. **Search the conversations.** Type a query with other words than the chat and
   select the result: the chat opens at the matching turn, highlighted.
   - `air trapped in the old heaters`: the radiator turn, the third of four
     turns about the 1920s apartment
   - `bathing rules at a Japanese hot spring`: the onsen turn of the Japan trip
   - `counting overnight hours in a spreadsheet`: the second turn of the pandas
     chat
   - `what to eat before race day`, `orange streaks on my photos`,
     `fungus on the tomato leaves`
2. **Let Leo search them.** In a new chat, ask
   `Find my old chat about the radiators in our apartment.`, select Allow once,
   and select the card: the radiator turn again.
3. **Learn from chats.** brave://settings/leo-ai/customization, Learn from chats
   (about 10 s): the review shows the seven facts, the preference for short
   answers in bullet points, and none of the five traps. Save. Then select the
   lock of the memory about the cello: Leo keeps it as it is.
4. **One memory changes.** In a new chat, type
   `We adopted a second cat last weekend, a rescued black kitten named Luna. How do we introduce her to Mochi?`
   Learn from chats again: the review has one change, "Changed": the memory
   about Mochi becomes "Has a Maine Coon cat named Mochi and adopted a black
   kitten named Luna". Save. brave://leo-memory-internals, Dreaming runs: the
   steps of that run, with each request to the local models and its answer; the
   relation step shows why the kitten joined the memory about Mochi.
5. **The memory in a chat.** Ask
   `We are away for a long weekend. What should the cat sitter know?` The reply
   is short, in bullet points. Under it, "Customization and memories sent" lists
   the preference and the memory about Mochi and Luna. The Chat turns tab of
   brave://leo-memory-internals shows how the turn chose them, in the step
   `relevance`: the preference got a yes for "follow it?" (`preferences`), and
   the memory about Mochi and Luna a yes for "relevant?" (`memories`), where the
   other memories got a no. The locked cello got a no too: a lock does not send
   a memory every time.

`rehearse` runs steps 2 to 5 on a copy and stops when one of them gives
something else. `verify` checks step 1. The models answer with temperature 0, so
the learned memories are the same in each run.

## Known issue

A debug-build DCHECK in Blink text shaping (`ng_shape_cache.h`,
crbug.com/486945341) crashes the renderer of `brave://leo-ai` within seconds,
even on a new profile. It is not in AI Chat code. To run the tools, comment out
the `DCHECK_EQ(*cached_result, *other_shape_result)` line locally and rebuild;
never commit that.
