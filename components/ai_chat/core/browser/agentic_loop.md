# Measuring the AI Chat agentic loop

The agentic loop is the tool-calling loop that drives Chromium's actor framework
when a conversation uses the content agent tools (agent profiles only, gated by
`kAIChatAgentProfile`). This document covers how to measure it. For the tools
themselves see [tools/tools.md](tools/tools.md).

## Where time goes

One "step" of the loop is a model round-trip plus one tool call, and the loop
runs steps serially until a response arrives with no tool use requests:

```
AIChat.ToolLoop                     first unanswered tool request -> loop finished
  AIChat.ToolUse                    one tool, dispatch -> output recorded
    AIChat.TabSetup                 resolve/create the task tab, add it to the actor task
    AIChat.Actuation                actor executes the action(s) and settles the page
    AIChat.Observation              page content extraction for the tool result
      AIChat.ObservationSerialization   proto -> content blocks
  AIChat.AssociatedContentExtraction    re-extraction of attached pages before generation
  AIChat.Generation                 request built and sent -> stream complete
```

`AIChat.Generation` and `AIChat.AssociatedContentExtraction` also happen outside
a tool loop, for ordinary conversations.

Phase names are declared once in [agent_tracing.h](agent_tracing.h) and used for
both sinks below, so a trace and a journal dump can be read side by side.

## Sinks

**Perfetto trace, category `brave.ai_chat`.** Always recorded, every platform.
Each phase is an async span on its own named track, carrying a `details`
argument of `key=value` pairs. Record with `chrome://tracing` or
[ui.perfetto.dev](https://ui.perfetto.dev), selecting the `brave.ai_chat`
category. This is the sink for wall-clock and nesting questions.

**Actor journal, `chrome://actor-internals`.** Desktop agent profiles only.
Entries land on the actor task's browser track, so Brave's phases interleave
with Chromium's own actor entries for the same task - which is how to tell
whether a slow `AIChat.Actuation` is Brave's dispatch or the actor's settle
delays. Journal entries are on-device only.

`components/ai_chat` can depend on neither `chrome/browser/actor` nor
`components/actor`, so the loop writes to the journal through the `AgentJournal`
interface in [agent_tracing.h](agent_tracing.h), which
`brave/browser/ai_chat/content_agent_tool_provider.cc` implements. A
`ToolProvider` that cannot reach a journal returns null from `GetAgentJournal()`
and only the trace spans are recorded.

## Details worth knowing

- `AIChat.ToolLoop` ends with `tool_uses=` and `generations=`: the step count
  and the number of model round-trips the loop added. `generations` counts the
  post-tool requests, not the request that started the loop.
- `AIChat.Observation` ends with `chars=`, the size of the page dump sent back
  as the tool result. `ConvertAnnotatedPageContentToBlocks` caps this at 100,000
  characters.
- `AIChat.ActionResult` instant events carry, per action, three things the actor
  produces that never reach the model: `duration_ms` (timed by the actor, not by
  us), `actor::ToDebugString` of the result including its English failure
  message, and the observation policies the loop ignores - it always extracts
  and never screenshots. `screenshot_policy=` and `extraction_policy=` print as
  integers: `0` skipped, `1` requested, `2` required. They are read from each
  `ActionResult` rather than from the `TabObservationStrategy`, which CHECKs
  unless locked and is only locked on the paths where actions actually ran.

## Comparing branches

Run a fixed three-task script - a search-and-filter flow, a form fill, and a
multi-page navigation - and capture both a trace and the journal for each. The
numbers to report:

1. wall-clock per task
2. model round-trips per task (`generations` on `AIChat.ToolLoop`)
3. prompt tokens per step
4. conversation tokens added per task, both the post-task prompt floor and the
   untrimmed residue - tool call arguments, results under
   `kContentSizeLargeToolUseEvent`, and database rows
5. task success

Chromium's settle delays (`actor-observation-delay-timeout`,
`glic-actor-page-stability-min-wait`, `actor-observation-delay-lcp`,
`actor-observation-delay-autofill-predictions-timeout`) are often the dominant
term in `AIChat.Actuation`. Tune them through Griffin rather than by editing
Chromium's defaults.
