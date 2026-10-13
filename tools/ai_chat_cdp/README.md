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
| `open_profile.py`        | Opens a test profile with `pnpm start`, to try it by hand                                            |
| `cdp.py`, `leo.py`       | The CDP client and the Leo helpers                                                                   |

## Profiles

A profile lives where `pnpm start --user_data_dir_name=<name>` puts it, so it is
outside the repo and outside `/tmp`, and a restart does not wipe it:

| Name                      | Path on macOS                                                         |
| ------------------------- | --------------------------------------------------------------------- |
| `leo-search-test`         | `~/Library/Application Support/BraveSoftware/leo-search-test`         |
| `leo-learned-memory-test` | `~/Library/Application Support/BraveSoftware/leo-learned-memory-test` |

On Linux the folder is `~/.config/BraveSoftware`. The state of the scripts, the
reports and the browser logs are in the `ai_chat_cdp` folder of the profile, so
deleting the profile deletes them too. `--profile <name>` picks another profile.

The scripts copy the embedder model (`BraveLocalAIModels`) from the profile of a
plain `pnpm start`. Start the dev build once with Settings > Privacy > Semantic
history search on, and wait until that folder exists.

## Before a run

- **Build first.** The scripts stop when the browser is older than the last move
  of HEAD (pull, rebase, checkout): a test of a binary without the branch says
  nothing.
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
and the eval reviews a waiting one before it selects "Dream now".

## Known issue

A debug-build DCHECK in Blink text shaping (`ng_shape_cache.h`,
crbug.com/486945341) crashes the renderer of `brave://leo-ai` within seconds,
even on a new profile. It is not in AI Chat code. To run the tools, comment out
the `DCHECK_EQ(*cached_result, *other_shape_result)` line locally and rebuild;
never commit that.
