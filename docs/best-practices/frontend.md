# Front-End Best Practices

<a id="FE-001"></a>

## ❌ Don't Spread Args in Render Helpers or Components

**Avoid spreading `...args` into `render()`, component props, or other React APIs.** Spreading arbitrary arguments can allow unexpected attributes to be injected, potentially leading to XSS. Pass explicit props instead.

```tsx
// ❌ WRONG - spreading arbitrary args
async function renderMyComponent(
  ...args: Parameters<typeof render>
): Promise<ReturnType<typeof render>> {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(...args)
  })
  return result!
}

// ✅ CORRECT - explicit props
async function renderMyComponent(
  ui: React.ReactElement,
  options?: RenderOptions
): Promise<RenderResult> {
  let result: RenderResult
  await act(async () => {
    result = render(ui, options)
  })
  return result!
}
```

---

<a id="FE-002"></a>

## ❌ Avoid Redundant React Keys

**When a parent component already assigns a `key` prop to a child in a list, the child should not redundantly set its own key on inner elements for list-keying purposes.** Redundant keys suggest a misunderstanding of React's reconciliation boundary.

```tsx
// ❌ WRONG - parent already provides key
{items.map((item) => (
  <AttachmentItem key={item.id}>
    <div key={item.id}>{item.name}</div>  {/* Redundant! */}
  </AttachmentItem>
))}

// ✅ CORRECT - key only on the list element
{items.map((item) => (
  <AttachmentItem key={item.id}>
    <div>{item.name}</div>
  </AttachmentItem>
))}
```

---

<a id="FE-003"></a>

## ✅ Move Utility Functions to Dedicated Modules

**Pure utility functions should live in dedicated utility modules (e.g., `utils/conversation_history_utils.ts`), not in React context/state files.** Context files should focus on state management, not data transformation logic.

---

<a id="FE-004"></a>

## ✅ Merge Similar UI Components

**When adding support for a new file type to an upload/attachment UI, merge similar components into a single generic one** rather than creating parallel components.

```tsx
// ❌ WRONG - separate components for each type
<AttachmentImageItem />
<AttachmentDocumentItem />

// ✅ CORRECT - single generic component
<AttachmentUploadItem type={file.type} />
```

---

<a id="FE-005"></a>

## ❌ Don't Redefine Types from Generated Bindings

**In TypeScript tests, import enum types from generated Mojo bindings or source files.** Do not redefine or duplicate them in test files.

```tsx
// ❌ WRONG - redefining enum from mojom
enum FileType {
  kImage = 0,
  kDocument = 1,
}

// ✅ CORRECT - import from generated bindings
import { FileType } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom-webui.js'
```

---

<a id="FE-006"></a>

## ❌ Avoid Unnecessary `useMemo` for Simple Property Access

**Don't wrap simple property lookups in `useMemo`.** Accessing `array.length`, `obj.property`, or other trivial derivations is cheaper than React's memoization overhead.

```tsx
// ❌ WRONG - useMemo for trivial access
const count = useMemo(() => items.length, [items])

// ✅ CORRECT - direct access
const count = items.length
```

---

<a id="FE-007"></a>

## ✅ Return Null from Components When No Data

**React components should return `null` early when there's no data to render,** rather than rendering empty containers or placeholder markup that adds unnecessary DOM nodes.

```tsx
// ❌ WRONG - renders empty container
function UserInfo({ user }: Props) {
  return <div className="user-info">{user ? user.name : ''}</div>
}

// ✅ CORRECT - return null when no data
function UserInfo({ user }: Props) {
  if (!user) return null
  return <div className="user-info">{user.name}</div>
}
```

---

<a id="FE-008"></a>

## ✅ Use `generateReactContext` for Mojo API Contexts

**Use the `generateReactContext` helper from `$web-common/api/react_api`** for creating React context + provider pairs for Mojo API bindings. Don't write custom context boilerplate for each API.

---

<a id="FE-009"></a>

## ✅ Use Partial Value Updates Instead of Spread-Then-Modify

**When updating state objects, prefer partial update functions over spreading the entire object and overriding one field.** This is more efficient and less error-prone.

```tsx
// ❌ WRONG - spread then override
setState({ ...state, isLoading: true })

// ✅ CORRECT - partial update
setPartialState({ isLoading: true })
```

---

<a id="FE-010"></a>

## ❌ Don't Provide Defaults for Leo CSS Variables

**Never provide fallback/default values for Leo design system CSS custom properties.** Leo variables are guaranteed to be set by the design system; providing defaults can mask theming bugs and produce inconsistent styling.

```css
/* ❌ WRONG - default masks theming bugs */
color: var(--leo-color-text-primary, #000);

/* ✅ CORRECT - trust the design system */
color: var(--leo-color-text-primary);
```

---

<a id="FE-012"></a>

## ✅ New UI Components Must Have Storybook Stories

**New UI features should have Storybook stories that cover their primary visual states.** Stories serve as both documentation and visual regression baselines. A single integrated story that exercises multiple closely-coupled components together (e.g., a panel story that covers the full panel with all sub-components) is sufficient — individual per-component stories are not always necessary. This is a preference, not a requirement — do not insist if the developer considers existing story coverage adequate.

---

<a id="FE-013"></a>

## ❌ TS Mojom Bindings Generate Interfaces, Not Classes

**TypeScript WebUI mojom bindings generate interfaces, not classes.** This means `instanceof` checks won't work on mojom types. Use type guards or discriminated unions instead.

```tsx
// ❌ WRONG - instanceof on mojom-generated type
if (value instanceof mojom.ConversationTurn) { ... }

// ✅ CORRECT - type guard or property check
if ('text' in value && 'role' in value) { ... }
```

---

<a id="FE-014"></a>

## ✅ WebUI Resource Files Must Be in Correct Top-Level Directory

**WebUI resource files must be placed in the correct top-level directory under `resources/` matching the WebUI host name.** Misplaced resources won't be found at runtime.

---

<a id="FE-015"></a>

## ✅ Prefer Semantic HTML Links Over Button+JS Navigation

**When a UI element's only action is navigating to a URL, use a proper `<a>` link element instead of a `<button>` with JavaScript navigation.** This provides better accessibility (users can see the destination on hover) and follows semantic HTML principles.

**Exception:** When a design system (e.g., Nala/Leo) provides styled button or link components, prefer using those for visual consistency with the design language. Design system consistency takes precedence over raw semantic HTML.

```tsx
// ❌ WRONG - button with JS navigation
<button onClick={() => window.open(url)}>Visit</button>

// ✅ CORRECT - semantic link
<a href={url} target="_blank" rel="noopener">Visit</a>

// ✅ ALSO CORRECT - design system component for navigation
<Button onClick={() => api.openTab(url)}>Settings</Button>  // When design system requires Button
```

---

<a id="FE-016"></a>

## ❌ Avoid Unnecessary `waitFor` in React Tests

**In React component tests, if you are using `rerender` to trigger updates, `waitFor` should not be needed** since `rerender` is synchronous. Similarly, wrapping DOM mutations in `act()` applies pending React updates. Using `waitFor` unnecessarily makes tests slower and can mask timing-related bugs.

---

<a id="FE-017"></a>

## ❌ Avoid Global State Assumptions for Component-Scoped Operations

**When a function accesses global state (like `window.getSelection()`), consider what happens if multiple instances of the component exist on the page.** If the function is component-scoped, pass a ref to the specific component instance instead of relying on global state.

---

<a id="FE-018"></a>

## ✅ Document Exported TypeScript Types

**Exported TypeScript types intended for use outside the component must have documentation comments** explaining their purpose. Complex union types and data shapes used as component APIs especially need explicit documentation for consumers.

---

<a id="FE-022"></a>

## ✅ Documentation Code Examples Must Be Valid

**Code examples in README files and documentation must be syntactically valid and compilable.** Developers copy-paste them. Invalid JSX (e.g., missing fragment wrappers for sibling elements, `onClick={fn()}` instead of `onClick={() => fn()}`) causes confusion and compile errors.

```tsx
// ❌ WRONG - invokes immediately, missing fragment wrapper
<div>Hello</div>
<button onClick={api.reset()}>Reset</button>

// ✅ CORRECT - arrow function handler, fragment wrapper
<>
  <div>Hello</div>
  <button onClick={() => api.reset()}>Reset</button>
</>
```

---

<a id="FE-023"></a>

## ✅ Use TypeScript Entry Points Instead of Inline Scripts

**When adding JavaScript to WebUI pages, always use compiled TypeScript entry points rather than inline `<script>` tags.** This gives you type checking, code analysis, and consistent bundling. Add additional entry points to the GN build configuration.

```gn
# ✅ CORRECT - additional entry point in GN
entry_points = [
  ["main", rebase_path("main.ts")],
  ["patches", rebase_path("patches.ts")],
]
```

---

<a id="FE-024"></a>

## ✅ Prefer Functional Components Over Class Components

**Write React components as functions, not classes.** Functional components with hooks are simpler, easier to test, and compose better. Class components require more boilerplate, have subtler lifecycle bugs, and are not compatible with hooks.

```tsx
// ❌ WRONG - class component
class UserCard extends React.Component<Props, State> {
  constructor(props: Props) {
    super(props)
    this.state = { isExpanded: false }
  }

  componentDidMount() {
    fetchUser(this.props.id).then(user => this.setState({ user }))
  }

  render() {
    return (
      <div>
        <span>{this.state.user?.name}</span>
        <button onClick={() => this.setState({ isExpanded: !this.state.isExpanded })}>
          Toggle
        </button>
      </div>
    )
  }
}

// ✅ CORRECT - functional component with hooks
function UserCard({ id }: Props) {
  const [isExpanded, setIsExpanded] = React.useState(false)
  const [user, setUser] = React.useState<User | undefined>()

  React.useEffect(() => {
    fetchUser(id).then(setUser)
  }, [id])

  return (
    <div>
      <span>{user?.name}</span>
      <button onClick={() => setIsExpanded(e => !e)}>Toggle</button>
    </div>
  )
}
```

The only valid exception is `React.Component` error boundaries, which still require a class component (`componentDidCatch` has no hook equivalent).

---

<a id="FE-025"></a>

## ⚠️ Beware `||=` Short-Circuit When Replacing `|=`

**Never replace `|=` with `||=` when the right-hand side has side effects.** The `||=` operator short-circuits: once the left side is truthy, the right side is never evaluated. If the right side calls a function with side effects, those effects will be silently skipped.

```typescript
// ❌ WRONG - fn() won't be called once isDirty is true
isDirty ||= updateFileTimestamps(file)

// ❌ ALSO WRONG - fn() skipped when isDirty is already true
isDirty = isDirty || updateFileTimestamps(file)

// ✅ CORRECT - function always executes, result accumulated
isDirty = updateFileTimestamps(file) || isDirty
```

Place the side-effecting call on the LEFT side of `||` to guarantee it always runs.

---

<a id="FE-026"></a>

## ❌ Don't Replace Hardcoded Initial CSS With Design Tokens That Load Asynchronously

**In WebUI HTML files, keep hardcoded initial background/foreground colors for the page body or root container.** Design tokens like `var(--leo-color-container-background)` are loaded asynchronously via JavaScript — replacing the hardcoded value causes a visible flash of unstyled content (FOUC) while the token loads.

```html
<!-- ❌ WRONG - Nala token isn't available yet at initial paint -->
<style>body { background: var(--leo-color-container-background); }</style>

<!-- ✅ CORRECT - hardcoded value matches the token's resolved value, avoids FOUC -->
<style>body { background: #111114; }</style>
```

Once the page's JavaScript loads and Leo tokens are initialized, the themed value takes over naturally. The hardcoded value is only visible during the brief initial paint.

<a id="FE-030"></a>

## ✅ Wrap `JSON.parse` in Try/Catch in React Render Paths

**`JSON.parse` calls on external or server data in React render paths (including `useMemo`) must be wrapped in `try/catch`.** Malformed JSON will throw a `SyntaxError` that crashes the entire component tree if uncaught.

```tsx
// ❌ WRONG - crashes component on malformed JSON
const config = useMemo(() => JSON.parse(rawData), [rawData])

// ✅ CORRECT - graceful fallback
const config = useMemo(() => {
  try {
    return JSON.parse(rawData)
  } catch {
    return defaultConfig
  }
}, [rawData])
```

---

<a id="FE-027"></a>

## ❌ Don't Use `render().toBeTruthy()` in React Tests

**`render()` always returns a truthy object, so `expect(render(<Component />)).toBeTruthy()` always passes.** Assert on actual rendered content instead.

```tsx
// ❌ WRONG - always passes, tests nothing
expect(render(<MyComponent />)).toBeTruthy()

// ✅ CORRECT - verify actual content
const { getByText } = render(<MyComponent />)
expect(getByText('Expected text')).toBeTruthy()
```

---

<a id="FE-028"></a>

## ❌ Avoid `dangerouslySetInnerHTML` for External Content

**Do not use `dangerouslySetInnerHTML` to render HTML from external or untrusted sources.** Instead, parse the data and render known safe elements using React components. If raw HTML rendering is unavoidable, sanitize it first and request a security review.

---

<a id="FE-029"></a>

## ✅ Clean Up Async Operations in React `useEffect`

**Async operations started in `useEffect` must be properly cleaned up to prevent state updates on unmounted components.** Use `AbortController` or a ref-based cancellation flag.

```tsx
// ❌ WRONG - no cleanup, state update on unmounted component
useEffect(() => {
  fetchData().then(setData)
}, [])

// ✅ CORRECT - AbortController cleanup
useEffect(() => {
  const controller = new AbortController()
  fetchData({ signal: controller.signal }).then(setData)
  return () => controller.abort()
}, [])
```

---

