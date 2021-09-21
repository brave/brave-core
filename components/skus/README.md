# skus
The `brave-core` integration to the Rust [SKUs SDK](https://github.com/brave-intl/br-rs/).

Uses `cxxbridge` for the bindings.

## Troubleshooting
If `cargo build` causes a problem, you might try the following in the shell you're building:
```
export CARGO_NET_GIT_FETCH_WITH_CLI=true
```
