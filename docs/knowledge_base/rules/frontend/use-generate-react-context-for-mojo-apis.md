---
name: brave/use-generate-react-context-for-mojo-apis
description:
  Create the React context and provider for a Mojo API binding with
  generateReactContext from $web-common/api/react_api, not with hand-written
  context boilerplate.
rubric: design
severity: should_fix
---

**Rule.** To make a Mojo API binding available through React context, use the
default export of `$web-common/api/react_api` (imported as
`generateReactContext` or `generateReactContextForAPI`), which returns the
`Provider` and the `useAPI` hook. Do not write a `React.createContext()`, a
provider component and a `useContext()` hook by hand for each API.

**Why.** The helper is the one implementation of that boilerplate: its hook
throws when used outside the provider, and its provider takes `overrides` for
tests and Storybook. Each hand-written copy repeats the code and lacks whichever
of these it forgets.

## Examples

Bad:

```tsx
const WelcomeApiContext = React.createContext<WelcomeApi | null>(null)

export function useWelcomeApi() {
  return React.useContext(WelcomeApiContext)!
}
```

Good:

```tsx
import generateReactContextForAPI from '$web-common/api/react_api'

export const { useAPI: useWelcomeApi, Provider: WelcomeApiProvider } =
  generateReactContextForAPI<WelcomeApi>()
```
