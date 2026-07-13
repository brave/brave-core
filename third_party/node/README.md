# third_party/node

This directory provisions two separate things Brave's build depends on:

1. **The Node.js runtime** — the `node` binary (and `npm`) used to run the
   browser's build tooling.
2. **npm-delivered build tools** — pinned CLI tools (currently `pnpm`) installed
   from npm into `node_modules/`.

Both are downloaded from upstream, repackaged into tarballs, uploaded to Brave's
build-deps bucket, and pulled into each checkout during `gclient sync`. The
tracked files here are the scripts and manifests that produce and pin those
tarballs; the downloaded trees and generated tarballs are git-ignored (see the
repo `.gitignore`).

### Rolling Node

```sh
# 1. Bump NODE_VERSION in download_node.py, then:
vpython3 download_node.py --clear
vpython3 package_node.py
```

## npm-delivered build tools

The tool set is pinned in [`package.json`](package.json), with
[`package-lock.json`](package-lock.json) pinning the exact resolved versions and
their integrity. Everything is installed into a single `node_modules/` tree.

- [`download_node_modules.py`](download_node_modules.py) runs `npm install` from
  `package.json` using whatever `node`/`npm` is on the system PATH, and
  refreshes the lockfile.
- [`package_node_modules.py`](package_node_modules.py) packs the `node_modules/`
  contents into `node_modules.tar.gz` and prints the object details to record.

Tools installed this way are run via their in-package entry point, e.g. pnpm:

```sh
node third_party/node/node_modules/pnpm/bin/pnpm.mjs --version
```

### Adding or rolling a tool

```sh
# 1. Pin (or bump) the tool in package.json, then:
vpython3 download_node_modules.py   # installs it and refreshes package-lock.json
vpython3 package_node_modules.py
```
