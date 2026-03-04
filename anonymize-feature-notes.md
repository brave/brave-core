# "Anonymize" Rewrite Option for Leo AI Tools

Adds an "Anonymize" option to the Leo AI tools context menu under the existing
"Rewrite" section. Defends against LLM-based deanonymization as described in
Lermen et al. (arxiv.org/abs/2602.16800) by addressing both attack vectors:

- **Stylistic**: normalizes vocabulary, sentence structure, punctuation, writing
  quirks
- **Semantic**: generalizes personally identifying details (names, locations,
  employers, job titles, dates, technical specializations, niche interests, life
  events)

## Brave-core changes (13 files)

### Command ID

- `app/brave_command_ids.h` -- `IDC_AI_CHAT_CONTEXT_ANONYMIZE 56246`

### Mojom

- `components/ai_chat/core/common/mojom/common.mojom`
  - `ANONYMIZE` appended to `ActionType` enum
  - `kAnonymize` added to `SimpleRequestType` enum (before `kCount`)

### String resources

- `components/resources/ai_chat_ui_strings.grdp` --
  `IDS_AI_CHAT_CONTEXT_ANONYMIZE` ("Anonymize")
- `components/resources/ai_chat_prompts.grdp` --
  `IDS_AI_CHAT_QUESTION_ANONYMIZE` (privacy-normalizing rewrite prompt)

### Context menu

- `chromium_src/chrome/browser/renderer_context_menu/render_view_context_menu.cc`
  - Added to `IsRewriteCommand()`, `GetActionTypeAndP3A()`,
    `IsCommandIdEnabled()`, `ExecuteCommand()`, `BuildAIChatMenu()`
  - Menu item appears after "Improve", before "Change tone" submenu

### Metrics

- `components/ai_chat/core/browser/ai_chat_metrics.h` -- `kAnonymize = 8` in
  `ContextMenuAction` enum
- `components/ai_chat/core/browser/ai_chat_metrics.cc` -- `kAnonymizeActionKey`
  added to `kContextMenuActionKeys` map

### Engine / API plumbing

- `components/ai_chat/core/browser/utils.cc` -- `ANONYMIZE` -> prompt mapping in
  `GetActionTypeQuestionMap()`
- `components/ai_chat/core/browser/engine/oai_message_utils.cc` -- `ANONYMIZE`
  -> `SimpleRequestContentBlock(kAnonymize)`
- `components/ai_chat/core/browser/engine/engine_consumer_conversation_api.cc`
  -- `ANONYMIZE` -> `kAnonymize` in `ActionToRewriteEvent()`
- `components/ai_chat/core/browser/engine/conversation_api_client.h` --
  `kAnonymize` in `ConversationEventType`
- `components/ai_chat/core/browser/engine/conversation_api_client.cc` --
  `{kAnonymize, "requestAnonymize"}` in `kTypeMap`
- `components/ai_chat/core/browser/engine/conversation_api_v2_client.cc` --
  `{kAnonymize, "brave-request-anonymize"}` in `kSimpleRequestTypeMap`

### Test

- `components/ai_chat/core/browser/engine/oai_message_utils_unittest.cc` --
  `ANONYMIZE` test param added

## aichat server changes (~/work/aichat, 3 files + 1 new)

- `aichat/protocol/leo_api_protocol.py`
  - `requestAnonymize` added to `Tag` enum
  - `RequestAnonymizeEvent` class (inherits `RequestRewriteEvent`)
  - Added to `ConversationEvent` discriminated union
- `aichat/conversation/base.py` -- `@dispatch` handler for
  `RequestAnonymizeEvent`
- `aichat/prompts/request_anonymize.py` (new) -- prompt module for augment
  pipeline
- `example/page_excerpt_anonymize.json` (new) -- test fixture

## How to test

1. Start aichat server:
   `cd ~/work/aichat && aws-vault exec aichat-bsg-sandbox-eks -- ./scripts/start-local.sh`
2. Build brave-core: `cd ~/work/test/src/brave && npm run build`
3. Start browser:
   `npm run start -- --ai-chat-server-url="http://127.0.0.1:8000"`
4. Select text on any page, right-click -> Leo AI tools -> Anonymize
5. Side panel path: opens Leo panel with anonymize prompt (works without server
   changes)
6. In-place rewrite path: rewrites in editable fields via `requestAnonymize`
   (requires server changes)

## Prompt text

"Rewrite the excerpt to resist deanonymization. Apply both stylistic and
semantic anonymization. Stylistic: normalize vocabulary, sentence structure,
punctuation, and any distinctive writing quirks into plain, neutral, generic
language. Semantic: replace or generalize any personally identifying details --
specific names, locations, employers, job titles, educational institutions,
dates, ages, technical specializations, niche interests, life events, and
distinctive opinions -- with generic equivalents (e.g. 'a company' instead of
naming one, 'a city' instead of naming one, 'a programming language' instead of
naming one). Preserve the core meaning and argument while making the text
indistinguishable from any other author. Include only the rewritten version in
your response."

## Threat model (arxiv.org/abs/2602.16800)

Lermen et al. demonstrate an ESRC (Extract-Search-Reason-Calibrate) pipeline
that deanonymizes pseudonymous users across platforms at scale: 68% recall at
90% precision. The attack works on **semantic content** (what you write about)
more than stylometry (how you write):

- **Biographical micro-data**: job title, employer, education, seniority
- **Interests and expertise**: programming languages, technical skills, hobbies
- **Spatiotemporal signals**: locations, dates, life events, career timeline
- **Social connections**: references to specific colleagues
- **Distinctive opinions**: niche views combinable into a fingerprint

The prompt addresses all of these by generalizing identifying details into
generic equivalents while preserving the argument's core meaning.

### Example transformation

**Input** (identifying):

> "...at Brave Software... marketing team... Thursday standup in San
> Francisco... senior Rust developer... since 2019... buddy Jake from the Berlin
> office..."

**Output** (anonymized):

> "...at a technology company... a non-technical team... a regular team
> meeting... a senior software developer... for several years... A colleague at
> another office..."

## Issues encountered during build

- `conversation_api_v2_client.cc` has a `static_assert` requiring
  `kSimpleRequestTypeMap` to cover all `SimpleRequestType` values -- needed
  `kAnonymize` entry added
- `ai_chat_metrics.cc` constructor iterates all `ContextMenuAction` values and
  looks them up in `kContextMenuActionKeys` -- missing entry caused a CHECK
  failure at startup
