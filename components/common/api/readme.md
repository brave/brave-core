# Interface API Provider

It's useful to have a distinction between state originating from a remote and
state which is considered local to your UI. For example, a remote might provide
information about the user's preferences or the state of a service, whereas the
UI might provide information about whether a navigation menu is currently open,
or which item of a list is currently being edited, pending updating the remote.

It's also useful to have a distinction between data provided from different
remote endpoints, and even different remotes, so that an overview of the flow
and modification of data can be more easily obtained.

This API introduces the remote API client part of that distinction.

A popular pattern has emerged to accopmlish this for many apps and websites,
using a store that specializes around fetching remote state. This pattern can be
seen in libraries like [_Redux Toolkit Queries_](https://redux-toolkit.js.org/rtk-query/overview) and [_TanStack Query_ (a.k.a.
_React Query_)](https://tanstack.com/query/latest/docs/framework/react/overview). The latter is the library used behind the scenes to manage and
query the store.

Remote APIs often have these things in common:

- They are fetched async, so have the concept of "in progress"
- They can error
- They can be queryable anytime, or they perform mutating actions (sometimes
  with a result we want to cache globally and sometimes not).

### Goals

- Simple to define from mojom interface (or extension API or URL fetch, etc...)
- Simple to understand conceptually, given an understanding of React and state
  management
- Minimal to use
- Performance optimized by default - encourages minimal re-rendering without
  overhead.

# createInterfaceAPI

Creates a store that is intended to proxy data between the client and a remote,
using ~React~ TanStack Query API helpers. These are supplemented with
React-specific hook wrappers (and helpers for other frameworks could be added).
It provides a way to take a simple definition of your interface's endpoints,
actions and events, generating both framework-independent accessors, and
mutators as well as React hooks which create easy subscriptions in components.

## Endpoints

```typescript
// Sample types
type Todo = { id: string, task: string, isComplete: boolean }

type MyInterface = {
  // No parameters
  getTodos: () => Promise<{ todos: Todo[] }>
  // Takes a parameter
  getTodoHistory: (id: string) => Promise<Todo[]>
  updateTodo: (id: string, task: string)
  markComplete: (id: string)
  deleteTodo: (id: string)
  addTodo: (task: string) => Promise<{ newTodo: Todo }>
}

type MyInterfaceObserver {
  OnTodosUpdated(Todo[])
}

// API usage
function createMyAPI(MyInterface myInterfaceRemote) {
  // Define the API layout and what to do with the interface data:
  // - which functions should be exposed to the UI as subscribable with hooks
  // - which should be prefetched,
  // - which are mutable and should only be called explicitly
  const api = createInterfaceAPI({
    // endpoints are subscribable Queries or Mutations.
    // - Queries are fetched as soon as the hook is present.
    // - Mutations only when the mutate function of the hook is called.
    endpoints: {
      // `endpointsFor` is a helper to generate endpoints from an interface
      // with minimal boilerplate. It uses typescript to infer as much as
      // possible about the function.
      ...endpointsFor(myInterfaceRemote, {

        // Todos is something we want fetched as soon as a component
        // needs it. It doesn't mutate anything on the remote.
        getTodos: {
          // Configure how to select the desired resopnse. We could do basic
          // conversion here, remove wrapper object, or just return as-is.
          response: (result) => result.todos
          // Optional: In fact, we want to fetch it as soon as possible and not wait for
          // the UI to mount.
          prefetchWithArgs: [], // there are no parameters for this function
          // Optional: So that the UI doesn't have to check if undefined.
          placeholderData: [] as Todo[]
          // ...or use any other option available to useQuery
          // https://tanstack.com/query/latest/docs/framework/react/reference/useQuery
          // e.g. refetch options / retry intervals
        },

        // Takes a parameter, inferred by the Typescript interface
        getTodoHistory: {
          response: (result) => result.todos,
          // No prefetch because we don't know which Id we want the hsitory
          // for, if any.
          // We can have placeholder Data still, for each useGetTodoHistory(id) call.
        },

        // This function will not be called automatically, only when
        // the UI calls the useXYZ().mutate() function.
        updateTodo: {
          mutationResponse: (result) => result.newTodo,
          // Optional: Handle when any part of the UI calls this,
          // perhaps for optimistic update or to report a global error
          // or success status.
          onSuccess: (result) => {
            // Perhaps we need to update the state here
            api.getTodos.update(old => [...old, result.newTodo])
          },
          // Optional: onMutate is called before the mutation function is fired and
          // can be used for optimistic updates
          onMutate: (task) => {
            // update something in the store optimistically...
          },
          // ...or use any other option available to useMutation
          // https://tanstack.com/query/latest/docs/framework/react/reference/useMutation
        }
      }),
    }
  })
}
```

We can now use the following helpers in our UI:

```tsx
function MyReactComponent(props: Props) {
  // The query
  // See https://tanstack.com/query/latest/docs/framework/react/reference/useQuery
  const {
    // The result of the most recent fetch for this endpoint, subscribed to updates
    getTodosData,
    // Whether the endpoint has received data from the first fetch yet.
    // Equal to `isFetching && isPending`.
    isLoading
  } = api.useGetTodos()

  // The mutation
  // See https://tanstack.com/query/latest/docs/framework/react/reference/useMutation
  const {
    // The function that will actually cause the mutation
    updateTodo,
    // The result of the most recent mutation on this endpoint,
    // often not needed.
    data: updateTodoResult,
    // Whether the update is pending
    isPending: isUpdateTodoSubmitting,
    // Which arguments were last passed to this function
    variables: updateTodoVariables,
  } = api.useUpdateTodo()

  return (
    {getTodosData.map(todo => (
      <div key={todo.id} class={isUpdateTodoSubmitting && updateTodoVariables?[0].id === todo.id ? 'updating' : ''}>
        <div>{todo.task}</div>
        <button onClick={() => updateTodo({ id: todo.id, task: 'moo' })}>
          Change to "Moo"
        </button>
      </div>
    ))}
  )
}
```

## Events

Continuing the previous example:

```ts

const api = {
  ...
  events: {
    ...eventsFor(MyInterfaceObserver, {
      onTodosUpdated(todos) {
        // Errors because typescript knows about onTodosUpdated
        todos.foo
        // We can update the state from here so that
        // all subscribers get the latest data instead of
        // having to invalidate and re-fetch
        api.getTodos.update(todos)
        // If we did want to force getTodos to re-fetch (once) and
        // all subscribers get the updated data, we could call
        api.getTodos.invalidate()
      }
    },
    (observer) => {
      // Wire up the binding
      service.bindObserver(new MyInterfaceObserverReceiver(observer).$.bindNewPipeAndPassReceiver())
    })
  }
}
...
```

...we now get the following optional helper to also handle the event in React...

```tsx
function MyReactComponent(props) {
  // We can also handle the event in React
  // by passing a handler function and listing the deps
  // for a useEffect.
  api.useOnTodosUpdated((todos) => {
    props.showToast(`Received ${todos.length} updated todos`)
  }, [props.showToast])
  ...
}
```

...but note that this isn't the best example because our API definition already
handles this and updates the data, which we can subscribe to and check for
changes. A better example could be if we defined an event not reflected in the
data, like `OnAuthenticationChanged(bool)`:

```tsx
const api = {
  ...
  events: ...
    // not handled here, but defined so we can create something to subscribe to
    onAuthenticationChanged(isAuthenticated) {}
}

function MyReactComponent(props) {
  api.useOnUnauthenticated(() => {
    props.showToast(`You must now re-authenticate`)
  }, [props.showToast])
}
```

## Actions

Sometimes we want to allow the UI to call actions exposed by mojo, that we don't
need to subscribe to the result, to the progress for or to expose calls from one
part of the UI to other parts. For example, perhaps the result is broadcast to
observers and isn't a long-running or awaitable action. We can pass the entire
interface but restrict access via Typescript to the functions we allow.

```tsx
const api = createInterfaceAPI({
  ...
  actions: myInterfaceRemote as Pick<MyInterface, 'markComplete' | 'deleteTodo'>
  ...
})


function MyReactComponent(props) {
  return <button onClick={api.deleteTodo}>Delete</button>

  // Typescript compile will fail!
  return <button onClick={api.addTodo}>
}
```

## Other uses

### Parameterized updates

Given the interface function

```ts
GetTodoHistory(task_id: string) => Promise<Todo[]>
```

We want to expose hooks to easily call that function given an ID, and get
updated whenever it is re-fetched or updated elsewhere.

```ts
const api = createInterfaceAPI({
  endpoints: ...
    // Takes a parameter, inferred by the Typescript interface
    getTodoHistory: {
      response: (result) => result.todos,
      // No prefetch because we don't know which Id we want the hsitory
      // for, if any.
      // We can have placeholder Data still, for each useGetTodoHistory(id) call.
    },
})
```

We only have to define it as a query like everything else and Typescript knows
that the function takes parameters. Whenever we call its hook, we need to
provide those parameters so we know what to subscribe to.

```tsx
function TaskHistory(props) {
  const { data: history, isLoading } = api.useGetTodoHistory(props.taskId)

  return (...)
}
```

### Query-less state

Sometimes state does not have a `getState` function and instead is _only_
provided by observer functions. Often the initial state is provided by a bind
function.

For this we can define a query without a query function:

```tsx
const api = createInterfaceAPI({
  endpoints: ...
    ...,
    // Type has to be specified.
    // Initial data is optional but prevents undefined checks in code
    state: state<Mojom.ServiceState>({
      hasAcceptedAgreement: false,
      isStoragePrefEnabled: false,
      isStorageNoticeDismissed: false,
      canShowPremiumPrompt: false,
    }),

    // No initial state provided, `data` could be undefined in the UI
    isAuthenticated: state<boolean>(),
    ...
})

// Some observer event updates the state and subscribers automatically update
...
function onAuthenticationChanged(isAuthenticated) {
  api.isAuthenticated.update(isAuthenticated)
}

// my_component.tsx
function MyReactComponent(props) {
  const { data: isAuthenticated } = api.useIsAuthenticated()

  if (data === undefined) {
    return <Spinner />
  }

  if (!data) {
    return <Blocked />
  }
}
...
```

# Passing the API to a React UI tree

The intention is to use a React Context Provider to pass an API instance
relevant to a branch of a UI tree, instead of the UI accessing via a global
singleton or prop-drilling. This aids mocking as well as having different
interface providers for different parts of a tree (e.g. multipe side-by-side
Conversations in a messaging app).

We can use the helper provided by react_api.tsx

```tsx
// state/my_feature_context.tsx

// Option 1: simply pass down the API instance
export const { useAPI: useMyFeature, Provider: MyFeatureProvider } =
  generateReactContextForAPI<ReturnType<CreateMyFeatureAPI>>()

// Option 2: Use React to pass down some local and derived state
export const { useAPI: useMyFeature, Provider: MyFeatureProvider } =
  generateReactContextForAPI((api) => {
    // Here we can put extra local or derived state relevant to this part of the tree
    // that is inter-dependent and benefits from react's Hooks
    const [isToolsMenuOpen, setIsToolsMenuOpen] = React.useState(false)

    const { getTodosData } = api.useGetTodos()

    const uncompletedTaskCount = React.useMemo(() => {
      return getTodosData.reduce(
        (total, task) => total + (task.isComplete ? 0 : 1),
        0,
      )
    }, [getTodosData])

    return {
      api,
      uncompletedTaskCount,
      isToolsMenuOpen,
      setIsToolsMenuOpen,
    }
  })
```

```tsx
// my_feature_page.tsx
import { MyFeatureProvider } from './state/my_feature_context.tsx'

function Page(props: { todoUserId }) {
  // Perhaps our API is read from the url state
  const api = React.useMemo(() => {
    const todoService = createTodoInterfaceForUser(todoUserId)
    return createMyFeatureAPI(todoService)
  }, [props.todoUserId])

  return (
    <MyFeatureProvider api={api}>
      <MyFeature />
    </MyFeatureProvider>
  )
}
```

```tsx
// components/my_feature.tsx
import { useMyFeature } from '../state/my_feature_context.tsx'

function MyFeatureComponent(props) {
  const myFeature = useMyFeature()

  const { getTodosData } = api.useGetTodos()

  return (
    <button onClick={() => myFeature.setToolsMenuOpen(!myFeature.isToolsMenuOpen)}>
      You have {myFeature.uncompletedTaskCount} tasks to complete!
    ...
  )
}
```

Note that the performance in the these examples that pass more than just the API
instance is suboptimal - every single component in the tree will be revisited
after the `getTools` query is fetched because our own context provider
subscribed to it. This is common for React which won't perform expensive
DOM manipulations, but we still want to minimize how often it happens. Anything
that is only used by 1 or 2 Components should be queried in the components
themselves and can be made re-usable with custom hook functions. This can also
be solved using an external store or a better version of React Context which
provides a subscribable selector: `UseContextSelector`.
