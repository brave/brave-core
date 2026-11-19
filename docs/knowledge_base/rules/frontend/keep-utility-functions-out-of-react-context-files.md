---
name: brave/keep-utility-functions-out-of-react-context-files
description:
  Pure utility functions live in dedicated utility modules, not in React context
  or state files, which hold state management only.
rubric: design
severity: nit
---

**Rule.** Do not define a pure helper, one that only transforms data, in a React
context or state file. Put it in a dedicated utility module (such as
`utils/conversation_history_utils.ts`) and import it from there. Context files
hold state management: the context, its provider and the hooks reading it.

**Why.** A data transformation buried in a context file mixes two concerns, is
hard to find for the next caller who needs it, and can only be reused or tested
by pulling in the context along with it.

## Examples

Bad:

```tsx
// state/conversation_context.tsx
function getTurnsAfter(turns: mojom.ConversationTurn[], uuid: string) {
  const index = turns.findIndex((turn) => turn.uuid === uuid)
  return index === -1 ? [] : turns.slice(index + 1)
}

export function ConversationContextProvider(props: React.PropsWithChildren) {
  // ...
}
```

Good:

```tsx
// utils/conversation_history_utils.ts
export function getTurnsAfter(turns: mojom.ConversationTurn[], uuid: string) {
  const index = turns.findIndex((turn) => turn.uuid === uuid)
  return index === -1 ? [] : turns.slice(index + 1)
}

// state/conversation_context.tsx
import { getTurnsAfter } from '../utils/conversation_history_utils'
```
