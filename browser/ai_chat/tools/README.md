# AI Chat Browser Tools

Place subclasses of ai_chat::Tool here that require access to things in {chrome,brave}/browser.

## Content Agent Tools

A set of tools whose purpose is to act on web content. These most often utilize the chromium actor framework to create a Task for a conversation, add 1 or more Tabs to that Task and perform actions against 1 of those Tabs.

These tools should be owned by the ContentAgentToolProvider which can own the shared state between these tools for a single conversation, and provide information about the current Task and available Tabs to all the related tools via ContentAgentTaskProvider.
