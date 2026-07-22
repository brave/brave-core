This wrapper is a temporary PATH shim for CI jobs that still invoke `npm`. It
will be removed once CI calls `pnpm` directly.

When the target `package.json` declares `devEngines.packageManager.name` to
`pnpm`, the wrapper translates supported `npm` commands to `pnpm` and runs them
instead. If the package manager field is missing or names another package
manager, the wrapper falls back to the real `npm` binary. This lets the wrapper
coexist with subprojects that still use `npm`.

`npm --prefix <dir>` is used to choose the target project. The wrapper maps that
to pnpm's `--dir <dir>` flag when redirecting to `pnpm`.

Before redirecting `npm install` or `npm ci`, the wrapper removes an existing
`node_modules` directory if it does not look like pnpm's layout. A pnpm layout
is identified by `node_modules/.pnpm`.

Global installs, such as `npm install -g pnpm`, always pass through to the real
`npm` binary.

CI adds this directory to `PATH` automatically. The wrapper excludes its own
directory only when looking up the real `npm`, so it does not recursively invoke
itself.
