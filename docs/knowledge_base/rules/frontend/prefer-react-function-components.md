---
name: brave/prefer-react-function-components
description:
  Write React components as functions with hooks, not classes; only error
  boundaries may still be class components.
rubric: design
severity: should_fix
---

**Rule.** Write React components as function components using hooks (`useState`,
`useEffect`). Do not add a component that extends `React.Component` or
`React.PureComponent`. The only valid exception is an error boundary, which
still requires a class because `componentDidCatch` has no hook equivalent.

**Why.** Functional components with hooks are simpler, easier to test, and
compose better. Class components need more boilerplate, have subtler lifecycle
bugs, and cannot use hooks.

## Examples

Bad:

```tsx
class UserCard extends React.Component<Props, State> {
  componentDidMount() {
    fetchUser(this.props.id).then((user) => this.setState({ user }))
  }

  render() {
    return <span>{this.state.user?.name}</span>
  }
}
```

Good:

```tsx
function UserCard({ id }: Props) {
  const [user, setUser] = React.useState<User | undefined>()

  React.useEffect(() => {
    fetchUser(id).then(setUser)
  }, [id])

  return <span>{user?.name}</span>
}
```
