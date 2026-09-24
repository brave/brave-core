# Tools in AI Chat

A tool is a function that can be called by the AI to perform a task.

Tools are defined in the `Tool` class but have the following variations:

### Self-executing

The tool implements `UseTool(input_json, callback)` where it verifies JSON
parameters and sends an array of message content blocks to the callback. The agentic loop will automatically and immediately call the tool upon receiving a tool use request with the tool's name.

Use for:
- Tools where there is no identifiable user data being sent
- Tools where the user has already opted-in for them to be used (i.e. enabled in the conversation)

### User interaction

By implementing `bool RequiresUserInteractionBeforeHandling()`, the Tool can prevent `UseTool` being called automatically after the AI makes a tool request. When this returns `true`, `ConversationHandler` will not execute the tool or continue the response to the AI until asked to do so by `ConversationHandler::UseTool(string tool_use_id)`. If the user submits another prompt before the UseTool is called (or completes), then the tool use request
will be removed (since the AI APIs don't like an unanswered tool use request).

Use for:
- Tools where the user needs to give permission first
- Tools where the user needs to make a choice first, e.g. select tabs before
creating a group

### Externally-executing (or non-executing)

The tool itself does not implement `UseTool`. Instead, some other module executes the result. For example, the Tool might be asking the user for some data, presented in a structured way. In such a case, the tool would also return `true` from `RequiresUserInteractionBeforeHandling()` and the UI would provide the result via `ConversationHandler::RespondToToolUseRequest(tool_use_id, output)`. Alternatively, there could be a tool that requires no output, for example a tool which provides structured JSON for consumption only, such as a local memory storage.

Use for:
- Tools which present the user an option but no other processing is required
- Tools which present a summary of actions in a pre-defined structure at the end of a task

## Ownership

Tools are provided by `ConversationHandler::GetTools()`, but can come from multiple sources:
- Simple static tools provided by `conversation_tools.h`
- Instances of tools that owned by the `ConversationHandler`
- Instances of tools owned by the Browser and passed via `AIChatService` to the `ConversationHandler`.
- Instances of tools owned by the Content and passed via `AssociatedContentDelegate` or `AssociatedContentManager` to the `ConversationHandler`.

## Metadata

Each Tool must define
- Name
- Description
- InputProperties

It's important to be verbose when defining what the tool can do and when it should be used to avoid over or under eager tool use requests by the AI.

Other optional properties can be found at `tool.h`.

## UI

See `components/ai_chat/resources/untrusted_conversation_frame/components/assistant_response/tool_event.tsx` for metadata you can provide for a tool.

At the minimum you should provide some human readable name or interactive content in `toolText`. You can also customize the `statusIcon` and `progressIcon`.
You can display extra data in `tooltipContent` which is useful for verbose logging of large blocks like image screenshots or DOM content.
