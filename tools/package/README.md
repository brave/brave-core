# gclient `dep_type: 'aws'` (proof-of-concept)

depot_tools' `gclient` only understands three dependency types: `git`, `cipd`
and `gcs`. Brave hosts build dependencies on its own buckets (e.g.
`brave-build-deps-public.s3.brave.com`), which today are installed out-of-band
by `tools/cr/install_extra_deps.py`. This PoC adds a first-class `aws` dep_type
so those archives can be declared directly in a `DEPS` file and resolved by a
normal `gclient sync`.

It is intentionally the **simplest, least disruptive** version: public buckets
over plain HTTPS (no AWS auth), `size_bytes` optional, and none of gclient's
`.gcs_entries` ledger / cross-sync clobber refinements. See the evolution notes
below.

## What's here

- **`gclient_aws.py`** — the implementation, dropped into `depot_tools`. Holds
  `AwsDependency` / `AwsWrapper` (the gclient glue, mirroring `GcsDependency` /
  `GcsWrapper`) and a testable download core (`install_object`, `is_installed`).
- **`apply.py`** — idempotently installs the above into a depot_tools checkout:
  copies `gclient_aws.py` and inserts two small edits — an `elif dep_type ==
  'aws'` branch in `gclient.py`'s dispatch and a schema branch in
  `gclient_eval.py`. Re-running is a no-op.
- **`gclient_aws_test.py`** — offline test: validates the schema patch and
  exercises download → verify → extract → sidecars (and a sha256 mismatch)
  against a throwaway localhost HTTP server.

## Try it

```bash
# From this directory. Patches vendor/depot_tools in place (idempotent) and
# runs the offline checks.
vpython3 gclient_aws_test.py   # or: python3 gclient_aws_test.py

# Apply only, to an explicit depot_tools:
python3 apply.py /path/to/depot_tools
```

`vendor/depot_tools` is gitignored and re-cloned when its pin changes, so the
patch is local and disposable — re-run `apply.py` after any re-clone.

## Declaring an `aws` dep

The dep shape mirrors `gcs`. In a `DEPS` file (e.g. `brave/DEPS`, which uses
`use_relative_paths = True`, so `..` in a path key resolves against `src/brave`
— letting an entry target `src/third_party/...`):

```python
deps = {
  "../third_party/some-tool": {
    "bucket": "brave-build-deps-public.s3.brave.com",  # host, or full https URL
    "dep_type": "aws",
    "objects": [
      {
        "object_name": "some-tool/linux-x64-1.2.3.tar.xz",
        "sha256sum": "….",
        # "size_bytes": 1234,          # optional
        # "output_file": "tool.bin",   # optional; non-archives are left as-is
        "condition": "host_os == \"linux\"",
      },
    ],
  },
}
```

`bucket` + `object_name` are joined into `https://{bucket}/{object_name}`
(an explicit `http://`/`https://` prefix on `bucket` is honoured — that's how
the test points at localhost). Each object is downloaded, sha256-verified,
extracted (tar/zip), and recorded with the same `.{prefix}_hash` /
`.{prefix}_content_names` sidecars gclient writes for `gcs`, so a re-sync is a
no-op when the recorded hash already matches.

## Status / scope

Verified: `gclient_eval` accepts the new type; a real `gclient sync` dispatches
to `AwsDependency`, downloads, extracts, and writes sidecars (rc 0); re-sync is
a no-op; sha256 mismatches fail the sync.

Deliberately out of scope for the PoC (evolve later):

- **Build integration.** `apply.py` is run by hand here. Next step is calling it
  right after the depot_tools checkout in `build/commands/lib/depotTools.js`, so
  every `npm run sync` gets the patched gclient before any DEPS is parsed.
- **Authenticated / private buckets.** Would need SigV4 (boto3, the `aws` CLI,
  or hand-rolled signing) and a credential story for bots vs. local devs.
- **`.gcs_entries`-style ledger** for removed-object clobbering across syncs.
  The per-object hash check already makes re-syncs no-ops and re-downloads on
  change; only whole-object removal isn't tracked.
- **Migrating `install_extra_deps.py`'s `EXTRA_DEPS`** (the rust-toolchain
  overlay) onto `aws` deps in `brave/DEPS`.
- **Scoping which gclient must be patched.** Declaring `aws` deps in `brave/DEPS`
  means every gclient that parses the brave solution must carry the patch; a
  dedicated buildtools solution would isolate that. Not needed for the PoC.
