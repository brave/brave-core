# Porting the Linux x64 ASan job onto recipes

> [!NOTE]
>
> A high-level analysis and roadmap. Section 6 records the decisions that have
> been taken and that the rest of the document assumes; Section 8 breaks the
> work into stages with dependencies and exit criteria; Section 9 lists what is
> deliberately out of scope. The designs inside a stage are sketches, not
> settled interfaces.

## 1. Why this job

`brave-browser-build-linux-x64-asan` is the smallest interesting build job we
have:

- A single platform, architecture and build type (`linux`/`x64`/`Release`), so
  no cross-compilation and no signing machinery.
- Nightly and non-blocking: a regression costs a nightly sanitiser run, not a
  release.
- No artefacts to sign, notarise, upload or publish. Post-processing is a Slack
  message, JUnit parsing and a GitHub issue.
- It nevertheless exercises the full spine of a browser build: checkout,
  dependency install, build configuration, `init`, `build`, four test suites, a
  network audit, and a sanitiser self-check.
- It already has two variants (`SCOPE=brave` and `SCOPE=chromium`) driven from
  one job definition, which is exactly the "one configuration, several builders"
  problem that a config system exists to solve.

It is therefore a good first consumer of everything the engine does not use
today: module **configs**, a **generated per-builder configuration**, the
**`runtest.py` harness**, and Python-side **reporting**.

## 2. What the job does today

The job is a `pipeline` job defined in
`brave-devops/jenkins/jobs/browser/brave-browser-build-linux-x64-asan.yml`. The
YAML holds the trigger, the parameters and the stage graph; nearly all of the
behaviour is in the `utils` shared library
(`brave-devops/jenkins/lib/vars/utils.groovy`, roughly 3,000 lines shared by
every browser job).

The stage table and the rest of this document say `pnpm`. The Groovy helpers
still spell those calls `npm`, but that is legacy: on CI they resolve through
`brave/build/npm_wrapper/npm_wrapper.py`, a PATH shim that translates `npm` to
`pnpm` whenever `package.json` asks for pnpm. The new pipeline calls `pnpm`
directly and does not rely on the shim.

| Jenkins stage      | What it does                                                                               | Where the knowledge lives          | Destination                                |
| ------------------ | ------------------------------------------------------------------------------------------ | ---------------------------------- | ------------------------------------------ |
| `start-node`       | Starts an `m8a.16xlarge` public build node from the channel AMI                            | `nodeManagement.groovy`            | Stays in Jenkins                           |
| `check-node`       | Waits for the node, Slack-notifies node issues                                             | `nodeManagement.groovy`            | Stays in Jenkins                           |
| `env`              | Derives `BUILD_TYPE`, `OUT_DIR`, `VERSION`, `SLACK_PREFIX`, build name                     | job YAML + `utils.getBraveVersion` | Recipe properties                          |
| `abort`            | Aborts superseded builds on channel branches                                               | `utils.checkAndAbortBuild`         | Stays in Jenkins                           |
| `checkout`         | Full clone of brave-core into `src/brave` (wipe workspace)                                 | `utils.checkoutBraveCore`          | Recipe module                              |
| `install`          | `pnpm install` in `src/brave` behind the socket-firewall proxy                             | `utils.install`                    | Recipe module                              |
| `config`           | Writes `src/brave/.env`: release flags, service keys, RBE settings, `is_asan=true`         | `utils.config*`                    | **Deleted** (Section 6)                    |
| `init`             | `pnpm run init --target_os= --target_arch=x64` when `src/chrome/VERSION` is absent         | `utils.init`                       | Recipe module                              |
| `build`            | `pnpm run build -- Release --channel=<c> --target=brave:all`, then relaxes `ptrace_scope`  | `utils.runBuild`                   | Recipe module + `mb`                       |
| `test-*`           | `pnpm run test -- <suite> Release …` per suite, gated on `SCOPE`                           | `utils.runTests`, `test.ts`        | **Harness + targets**                      |
| `audit-network`    | `pnpm run network-audit -- Release …`                                                      | `utils.networkAudit`               | **A suite entry**                          |
| `test-asan`        | Runs one disabled `base_unittests` case and greps the XML for the expected ASan error      | `utils.verifyAsan`                 | Recipe step                                |
| `post` (per suite) | `junit(...)` over the report list in `src/<suite>.txt`                                     | `utils.parseTestReports`           | Harness writes, recipe checks, one glob    |
| `post` (job)       | S3 upload listing, ASan finding scrape, Slack, GitHub issue, node teardown, `.env` cleanup | `report/s3/gh/utils`               | **Python** (issues), Jenkins (node, Slack) |

### 2.1 Inputs and environment

Parameters: `SCOPE` (`brave`/`chromium`), `CHANNEL`
(`nightly`/`beta`/`release`), `BRANCH` (default `master`), `USE_RBE` (default
true), `NODE_LABEL`, `SLACK_NOTIFY`. Two `parameterized-timer` entries fire at
20:00 daily, one per scope, both on `master`/`nightly`, both reporting to
`#browser-sanitizers-bot`. The job times out after 270 minutes; `runBuild`
carries its own six-hour timeout, so the job timeout is the real limit.

RBE mTLS material arrives as `RBE_tls_client_auth_cert` /
`RBE_tls_client_auth_key` credentials in the stage environment. Brave service
keys and OAuth client secrets are injected by `withCreds("linux")` and written
into `.env` by `utils.config` / `utils.configServiceKeys`.

### 2.2 The build

`.env` is the single channel through which the job configures the build. For
this job it ends up containing, in order:

- from `configChromium`: the Chromium repository URL, and a depot_tools revision
  when pinned;
- from `config`: `is_brave_release_build=1`,
  `use_brave_hermetic_toolchain=true`, the API keys (real values on this job,
  `dummy` otherwise);
- from `configServiceKeys`: per-channel service keys;
- from `configBuildAcceleration`: `use_remoteexec=<USE_RBE>`, `rbe_service`,
  `rbe_jobs_limit=1440`, `siso_cache_dir`;
- from `configAsan`: `is_asan=true`.

Two consequences of the current GN-arg derivation matter for parity:

- `isOfficialBuild()` is false for any sanitiser build, so this is a
  non-official `Release` build, and `dcheck_always_on` is force-disabled for
  sanitisers;
- `isLsan()` is hard-coded to `false` pending brave-browser#56047, so we build
  ASan **without** LSan, whereas upstream's `Linux ASan LSan Builder` runs both.

`fail_on_san_warnings` deserves a paragraph of its own, because the obvious
reading of upstream's builder config is wrong. Upstream lists it explicitly, and
we never set it — but in `build/config/sanitizers/sanitizers.gni` it defaults to
`using_sanitizer`, so it is **already true in our ASan build**. Nothing is
missing from our GN args.

What the arg does is add `tools/memory/sanitizer/escalate_sanitizer_warnings.py`
to the test binary's `data`. The consumer is `testing/test_env.py`: when a test
command carries `--fail-san=1`, it runs that script over the test-launcher JSON
summary _after_ the tests, finds any test whose output contains
`SUMMARY: …Sanitizer:`, and rewrites its status from success to failure. So a
sanitiser report that does not itself crash the test still fails the run.

Our pipeline does not go through `testing/test_env.py`, so nothing escalates.
The gap is therefore on the test-running side, not the build side, and today it
is half-covered from the outside: `report.reportAsanFindings()` greps the
console log for the same `SUMMARY: AddressSanitizer:` pattern and opens an issue
— but the build still passes. That is the real difference from upstream, and it
is squarely in the harness's territory.

### 2.3 The tests

`brave:all` is built once, then each test stage invokes `pnpm run test`, which
compiles its own targets before running them (`test.ts` sets
`config.buildTargets` and calls `util.buildTargets`). Compile and test are
therefore coupled per suite today.

Knowledge currently spread across `utils.groovy`, `test.ts` and `testUtils.js`:

- suite lists per scope: `brave_all_unit_tests`, `brave_browser_tests`,
  `brave_interactive_ui_tests`, `brave_network_audit_tests` for `brave`;
  `chromium_unit_tests`, `browser_tests` for `chromium`;
- **group expansion**: a suite name is resolved through `<out>/<suite>.json`, a
  list of the group's deps mapped to executable names by stripping the
  `//path:target(toolchain)` decoration. Those files come from `write_file()` in
  `brave/BUILD.gn`, so they are written at **gen** time, not build time, and
  only `chromium_unit_tests` and `brave_all_unit_tests` have one — every other
  suite falls back to a single binary of the same name;
- **filter files**:
  `brave/test/filters/<suite>[-<platform>[-<arch>|-asan|-ubsan]].filter`, all
  matching files joined with `;` into `--test-launcher-filter-file`;
- **sanitiser environment**: `ASAN_OPTIONS=detect_odr_violation=0` (plus
  `detect_leaks=1` under LSan) and
  `LSAN_OPTIONS=suppressions=brave/test/sanitizers/lsan_suppressions.cfg`, set
  only for test launching, never for the build;
- launcher flags: `--enable-logging=stderr`, `--v=`, `--test-launcher-jobs`
  (`nproc`), `--test-launcher-bot-mode`, `--gtest_shuffle` for the two Brave
  gtest suites, `TEST_PREMATURE_EXIT_FILE`, `LLVM_PROFILE_FILE`, cwd set to the
  output directory, `continueOnFail` in CI;
- report plumbing: `--gtest_output=xml:src/<suite>.xml` and an index file
  `src/<suite>.txt` listing each XML, which is what `junit` consumes;
- per-suite default args: `brave_network_audit_tests` gets
  `--ui-test-action-timeout=320000 --test-launcher-timeout=2200000`
  (`pnpm run network-audit` is nothing more than
  `test brave_network_audit_tests`);
- per-suite timeouts in `getTestTimeoutMins` (30 to 240 minutes);
- `brave_interactive_ui_tests` gated on the `brave_interactive_ui_tests` entry
  in `brave/build/.ci_features`.

`verifyAsan` is a separate thing: it runs
`ToolsSanityTest.DISABLED_AddressSanitizerLocalOOBCrashTest` from
`base_unittests` with `--gtest_also_run_disabled_tests` and asserts that the XML
records exactly one error for that case. It is the check that the sanitiser is
actually instrumented and reporting, and it must never be allowed to pass
silently.

### 2.4 Reporting

`report.reportAsanFindings()` scrapes the **Jenkins console log** for
`SUMMARY: AddressSanitizer: …`; `gh.createTestIssuesOrComments()` walks the
**JUnit plugin's `TestResultAction`** for failed tests, filters out
sanitiser-caused failures, and opens or comments on issues in
`brave/brave-browser` (or `brave/internal` for findings) with bot labels,
previous-issue back-links and assignee inheritance from closed issues. Both are
therefore coupled to Jenkins' own data model, which is the main reason the port
is worth doing: once the recipe runs the tests itself, it holds better data than
the plugin does.

## 3. What the engine has today

`tools/recipes` (see its `README.md`) is a compact re-implementation of the
upstream recipe engine: DEPS resolution, module instantiation, typed properties
from protobuf, placeholders, simulation tests with committed expectations, and a
100% coverage gate.

Modules present: `brave_core_checkout`, `chromium_checkout`, `context`,
`depot_tools`, `env`, `file`, `hello`, `json`, `osx_sdk`, `path`, `platform`,
`proto`, `raw_io`, `step`.

Recipes present: `toolchains/{rust,windows,xcode}`, `tools/ast_grep`,
`tools/node/*` — all _packaging_ recipes producing an artefact from a narrow,
sparse checkout. Nothing builds a browser, and no module knows about GN, ninja,
`pnpm run build`, tests or GitHub.

Two assets shorten the work materially:

- **`config.py`** — the full upstream config machinery (`ConfigGroup`,
  `ConfigList`, `Single`, `Static`, `Enum`, `List`, `Set`, `Dict`,
  `config_item_context`, `is_root`/`group`/`includes`/`deps`, `BadConf`) plus
  `set_config` / `apply_config` / `make_config` and `get_config_defaults`
  wiring. Its only consumer is `hello`, which exists to document it.
- **`runtest.py`** with `gtest_output.py` — a gtest harness lifted from
  upstream. It already handles `--enable-asan`/`--enable-lsan`, `ASAN_OPTIONS` /
  `LSAN_OPTIONS` composition, the external symbolizer and `asan_symbolize.py`
  piping, xvfb plus DBus on Linux, the SUID sandbox path, and both stdout and
  JSON-summary parsing. No module calls it yet.

For the reporting port there is a house pattern to follow: `tools/cr/gh_cli.py`,
the `GhCli` class `brockit` uses. Every interaction with GitHub is a subprocess
call to the `gh` command-line tool, argument construction and JSON parsing sit
in one thin wrapper, Brave-specific policy (labels, assignees, how an issue is
matched by title) stays in the callers, and the whole thing is tested by
substituting the runner rather than by talking to GitHub. The Groovy reporting
already shells out to `gh` as well, so there is no authentication work either:
`gh` reads `GH_TOKEN` from the environment, which Jenkins binds from the
`gh-token-pr` credential exactly as it does today.

## 4. How Chromium does it, and what to borrow

Upstream has moved per-builder knowledge out of recipes and into starlark,
generating JSON that both `mb` and the recipes consume. This is not a finished
migration to admire from a distance — it is the design we are copying, so it is
worth being precise about its mechanics.

### 4.1 The chain, for our reference builder

1. **Definition** —
   `src/infra/config/subprojects/chromium/ci/chromium.memory.star` declares
   `Linux ASan LSan Builder` with a `builder_spec` (gclient config `chromium`;
   chromium config `chromium_asan` plus `apply_configs = ["lsan", "mb"]`;
   `RELEASE`; `target_bits = 64`; `target_platform = LINUX`) and, separately,
   `gn_args.config(configs = ["asan", "lsan", "fail_on_san_warnings", "release_try_builder", "minimal_symbols", "remoteexec", "linux", "x64"])`.
   Note what the builder does **not** contain: any GN arg values. It names
   configs.
2. **The configs** — those names are defined once, centrally, in
   `src/infra/config/gn_args/gn_args.star`. Each is a
   `gn_args.config(name = …, args = {…}, configs = […], args_file = …)`.
3. **Generation** — a `lucicfg.generator` resolves each builder's configs and
   writes `generated/builders/<bucket>/<builder>/gn-args.json`, plus a
   `gn_args_locations.json` index mapping builder group → builder → file path.
   Alongside it: `properties.json` (recipe input properties) and
   `targets/<group>.json` (test specs).
4. **Consumption** — `tools/mb/mb_config.pyl` lists `gn_args_locations_files`,
   so `mb lookup` / `mb gen` resolves a builder straight to its generated
   `gn-args.json`. The recipe calls `api.chromium.mb_gen(...)`, which runs
   `mb_lookup` first so the args are in the log even when the build fails.
5. **Verification** — `PRESUBMIT.py` re-runs the generator and fails if the
   checked-in output differs.

### 4.2 How `gn_args.star` composes

The library behind it is
`chrome_infra/chromium_infra/starlark-libs/chromium-luci/gn_args.star`, 300
lines worth reading in full. Its semantics, which we intend to reproduce:

- **A config is a node** with three inputs: `args` (a dict of GN arg values),
  `configs` (names of other configs to include), and `args_file` (the path of
  one `.gni` to import).
- **Resolution is a depth-first walk of the include graph**, memoised, merging
  children before the node itself. So a config's own `args` beat anything it
  includes.
- **Within `configs`, later beats earlier.** The library says it plainly: "an
  included config with a larger index in the list will overwrite duplicated GN
  args for any config with a smaller index." Order in the list is the conflict
  resolution rule — there is no error on a duplicate, and no need for one.
- **Exactly one `args_file` may survive a resolution.** Two configs each
  contributing one is a hard failure:
  `"Each GN config can only contain a single args_file"`. This matters for us
  (§7.3).
- **`target_os` and `target_cpu` are mandatory** in the resolved args, enforced
  at generation time.
- **`builder_defaults`** carries args derived from _non_-GN-arg builder
  properties — upstream uses it so that a builder's `use_siso` setting produces
  `use_siso = true` / `use_reclient = false` — and they are overridden by any
  named config, so a config can always opt out.
- **A validation hook** runs over each resolved builder. Upstream uses it to
  assert that builders in the public `chromium` group carry no proprietary
  codecs; the shape is general.
- **Phased configs** are supported: a dict of phase name → config generates
  `{"phases": {…}}` for `mb --phase`. We have no use for phases, but the shape
  is there if a builder ever needs two generations.

The resolved output per builder is small and flat:

```json
{ "args_file": "//build/args/chromeos/amd64-generic.gni", "gn_args": { … } }
```

### 4.3 What to copy, and what not to

Copy: **configuration is data, generated from named composable configs**;
**`lookup` is separate from `gen`**, so resolved args are always visible; **the
generated output is reviewed in code review**; and the composition semantics
above, down to the override order and the single-`args_file` rule, so that
anyone who knows one system knows the other.

Do not copy the layer upstream is _leaving_: `mb_config.pyl`'s own
`configs`/`mixins` tables, and the legacy GN args in
`recipe_modules/chromium/config.py`. The direction is measurable in the tree —
186 builders now have a starlark-generated `gn-args.json`, and the 26 builder
groups still listed in `mb_config.pyl` are almost entirely chrome-branded,
internal, or other projects mirroring into chromium's mb; none of
`chromium.memory`, `chromium.linux`, `chromium.mac`, `chromium.win` or
`tryserver.chromium.*` appear there any more. The same has happened to test
specs: 26 `testing/buildbot/*.json` files remain against 217 starlark-generated
`targets/*.json`. We are starting after that migration, so we never build the
thing being deleted: our `mb` reads generated data only, with no `configs` table
and no fallback lookup path.

Also not carried over: everything that assumes LUCI — buildbucket, Swarming,
ResultDB, CAS, sharding, separate tester builders — has no counterpart on a
persistent Jenkins node, so compile and test share one workspace for now (see
Section 9 for what that defers).

## 5. How Brave composes build configuration today

`build/commands/lib/` resolves configuration in three layers, and the
distinction matters for what "do away with `.env`" means:

1. **`package.json` `config`** — checked-in defaults: the Chromium repository
   URL, the `depot_tools` directory and repository, project
   revisions/tags/branches, the Brave version. Read through `EnvConfig` but
   **not** part of `.env`.
2. **dotenv (`src/brave/.env`)** — machine-local for developers, written by
   `utils.config*` on CI. Overrides layer 1. Also carries `gn_args_<name>=…`
   entries which land in `config.extraGnArgs` (advertised as the `env_gn_args`
   entry in `brave/build/.ci_features`).
3. **command line** — `--target_os`, `--target_arch`, `-C`, repeated
   `--gn key=value`, and so on.

`config.ts` merges those into a `Config`; `buildArgs.ts` then composes
`args.gn`: ordered `import("//brave/build/args/*.gni")` lines (`brave_defaults`,
`blink_platform_defaults`, branding, desktop/android/ios), then computed values
(`is_asan`, `is_lsan`, `enable_full_stack_frames_for_profiling`,
`v8_enable_verify_heap`, `is_official_build`, `brave_channel`, `use_remoteexec`,
…), then a forwarded allow-list of env-config variables
(`FORWARD_ENV_CONFIG_VARS_TO_GN_ARGS`), then sanitiser fixups. `build.js` runs
`gn gen` unless `use_no_gn_gen` is set — an existing seam for "somebody else
generated `args.gn`".

So: `pnpm run init` / `sync` / `build` need layers 1 and 3 to keep working, and
the new pipeline needs layer 2 gone. That is the shape of Decision D2 below.

## 6. Decisions taken

| #       | Decision                                                                                                                                                                                                                                                                                                                                                                  |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **D1**  | The new pipeline lands as a **parallel Jenkinsfile / job**. The existing job is untouched until cutover, and both run against the same branch during the shadow period.                                                                                                                                                                                                   |
| **D2**  | **No `.env`.** Passing `--builder <name>` disables the dotenv layer entirely — not merely ignores individual keys. The `package.json` layer stays; the builder config overrides it.                                                                                                                                                                                       |
| **D3**  | Tests run through **our harness** (`tools/recipes/runtest.py`). `pnpm run test` and `pnpm run network-audit` are retired for this pipeline.                                                                                                                                                                                                                               |
| **D4**  | `pnpm run init`, `pnpm run sync` and `pnpm run build` **stay** and remain the only way the pipeline syncs and compiles.                                                                                                                                                                                                                                                   |
| **D5**  | A new **`mb`-like tool** owns build configuration. Under `--builder`, `buildArgs.ts` is bypassed **entirely**: the generated `gn-args.json` is the input and `args.gn` is the output. It reads generated data only — no `mb_config.pyl` equivalent, which is the layer upstream is deleting.                                                                              |
| **D6**  | The **`.gni` defaults under `brave/build/args/`** stay where they are and remain imported. Only the per-builder overrides move into generated data.                                                                                                                                                                                                                       |
| **D7**  | **Secrets are resolved on the server** and materialise as one generated `.gni` in the **output directory**, imported by `args.gn`. Checked-in configs contain env-var names, never values.                                                                                                                                                                                |
| **D8**  | **GitHub issue reporting is ported to Python**, driven by the recipe's own test results rather than the Jenkins JUnit plugin model. GitHub is reached by subprocess calls to the `gh` CLI, as `tools/cr/gh_cli.py` does for `brockit` — not through a REST client.                                                                                                        |
| **D9**  | The generator is **Python**. Starlark is not realistic now, so the _spec_ files are written to mirror `gn_args/gn_args.star` and `chromium.memory.star` as closely as Python allows — declarative calls, keyword-only arguments, literals — so that porting later means replacing the host, not rewriting the spec (§7.4).                                                |
| **D10** | **GN args are composed the way upstream composes them**: one central file of named, includable configs — our `gn_args.star` — referenced by name from builders. Same resolution rules, including later-wins-within-`configs` and a single `args_file` per resolution (§4.2).                                                                                              |
| **D11** | **`--builder` is authoritative and exclusive.** Any switch whose value the builder config owns — `--target_os`, `--target_arch`, `--target_environment`, `-C`, `--channel`, the build-config argument, `--gn`, `--is_asan`/`--is_ubsan`, `--use_remoteexec`, the signing identities — is **rejected** when `--builder` is present. Not ignored, not merged: a hard error. |
| **D12** | **One builder per variant, as upstream does.** `SCOPE` stops being a parameter: `linux-x64-asan-brave` and `linux-x64-asan-chromium` are two builders, each with its own generated configuration. Names are kebab-case, since a builder name is also a directory name and a Jenkins parameter value.                                                                      |
| **D13** | **The output directory is `out/<builder>`.** It keeps a builder's tree away from a developer's `out/Release` and gives the secrets `.gni` a predictable home. Taken for now and deliberately cheap to revisit: it is one value in the builder config, read by `mb` and by `pnpm run build`, and nothing else depends on the spelling.                                     |
| **D14** | **Test results are still reported to Jenkins, but nothing in Groovy computes them.** The harness writes the JUnit XML, the recipe checks it is complete, and the job publishes it with a single fixed glob — the one part that cannot move, since only the JUnit plugin can ingest into the build UI.                                                                     |
| **D15** | **Slack stays in Jenkins** for the first cutover. Whether it follows GitHub into Python is a later, separate decision; the recipe publishes what a message would need rather than sending one.                                                                                                                                                                            |
| **D16** | **The `build/.ci_features` gate is kept** for `brave_interactive_ui_tests`, read from the checkout rather than fetched over HTTP. It may well be gone before A6 is implemented, so it is one field on a suite entry and its removal is a data change.                                                                                                                     |
| **D17** | **Align with upstream on sanitiser-warning escalation.** A sanitiser report that does not crash its test must fail the run. Upstream gets this from `testing/test_env.py`; we get it in the harness. `fail_on_san_warnings` is also set explicitly in the builder args, though it is already true by default.                                                             |
| **D18** | **LSan stays disabled** until the pipeline is stable, then gets investigated on its own (brave-browser#56047). It moves from a hard-coded `return false` in `config.ts` to `is_lsan = false` in the `asan` config in `gn_args.py`, so enabling it later is a data change on one builder.                                                                                  |
| **D19** | **A builder declares the test targets it runs**, not the groups it expands. The list is per-builder data, phrased in terms of the targets that will become isolates; the gen-time `<out>/<suite>.json` becomes a drift check against it rather than the source of truth.                                                                                                  |
| **D20** | **`brave/infra/config/gn_args.py` is the analogue of `gn_args/gn_args.star`** — the only place GN arg values are written — and the per-group builder files are analogues of `chromium.memory.star`: a builder is a name plus the configs it references. The generated file keeps upstream's `{args_file, gn_args}` shape plus a `secrets` map.                            |
| **D21** | **Config data is frozen once defined**, via a `freeze`/`thaw` pair lifted from upstream's recipe engine (`recipes-py/recipe_engine/engine_types.py`). This reproduces in Python the property starlark gives for free — a spec cannot mutate what it has already declared — so the eventual port is behaviour-compatible, not merely textual.                              |

Consequences worth stating explicitly:

- D2 plus D5 mean the builder config must carry **everything the build path
  needs**, not just GN args: target OS and architecture, build config, channel,
  output directory, RBE and siso settings, gclient overrides for `init`/`sync`,
  the secret list, the compile targets and the test suites. Auditing every
  `envConfig.*` call site is therefore a prerequisite, not a detail (Appendix
  C).
- D11 closes the last two ways a build could be described in more than one
  place. With dotenv off (D2) and conflicting switches rejected, the builder
  config is the whole truth: there is no `--gn key=value` on the command line,
  no `gn_args_*` entry in a file, and no `--target_arch` quietly disagreeing
  with the builder's own architecture. A CI invocation is then reducible to a
  builder name plus a ref, which is what makes the shadow comparison in B6
  meaningful. It also means there is deliberately **no escape hatch**:
  perturbing a builder's args requires editing the checked-in config or adding a
  config. A developer who wants a one-off variation runs `mb lookup <builder>`
  into a build directory, edits `args.gn` by hand and builds without
  `--builder`, using the existing `use_no_gn_gen` path. `mb` itself must not
  grow an `--extra-gn-arg` flag for the same reason.
- D3 means the knowledge inside `test.ts` and `testUtils.js` has to be ported,
  not discarded — group expansion, filter files, sanitiser environment and
  report plumbing (Appendix B). It also means compiling test targets becomes an
  explicit `pnpm run build --builder <b> --target <suite>` step, because
  `pnpm run test` used to do it.
- D7 has a pleasant side effect: because `args.gn` only ever contains an
  `import(...)` line for secrets, `mb lookup` output and the `args.gn` in the
  log are safe to print — which is more than can be said for today's `.env`.
- D8 removes the dependency on the JUnit plugin's model, so publication of JUnit
  XML in Jenkins becomes a convenience for the web UI rather than the source of
  truth for reporting.
- D19 settles what looked like an ordering problem, and it rests on a detail
  worth stating plainly: `<out>/<suite>.json` is produced by `write_file()` in
  `brave/BUILD.gn`, so it exists after **gen**, not after compilation. The
  recipe could therefore read it straight after `mb gen`. It should not treat it
  as the source of truth anyway: which tests a builder runs is a property of the
  builder, belongs in its `targets.json`, and wants phrasing in terms of
  individual test targets, because those targets are what become isolates when
  we need sharding or off-node execution. Groups stay useful as compile targets;
  they stop being the run list. Declaring the list does introduce a way for it
  to drift from `brave/BUILD.gn` — someone adds a test to
  `chromium_unit_tests_deps` and no builder runs it — so the gen-time file earns
  a second life as a check: compare it against the declared targets and fail on
  a difference. Drift becomes a build failure rather than a test nobody notices
  is unrun.
- D17 is the **one intended behaviour change** in this migration, so it should
  be landed visibly rather than folded into the port. Today a sanitiser report
  that does not crash its test leaves the build green and merely opens an issue;
  afterwards it fails the test that produced it. Expect exactly that class of
  divergence during the B6 comparison — a build the old path passes and the new
  path fails, for a finding both paths reported — and treat it as the change
  working, not as a regression. The detection is a single regex over test output
  (`SUMMARY: <name>Sanitizer:`), so the harness should detect once and hand the
  result to both consumers: the run's verdict, and the GitHub issue in B4. Note
  also that we do not need `--fail-san=1` or `escalate_sanitizer_warnings.py`
  themselves; we need what they do, and our harness already parses the same JSON
  summary they rewrite.
- D18 does not change behaviour today, but it changes where the decision lives.
  `isLsan()` returning a hard-coded `false` is invisible from CI configuration;
  `is_lsan = false` in the `asan` config in `gn_args.py`, with the bug number
  beside it, is a per-builder choice someone can flip on one builder to
  investigate without touching TypeScript. That is the whole argument for moving
  build configuration into data, on the smallest possible example.
- D16 keeps the gate but drops half of it. `isCiFeatureSupported` has two paths:
  read `src/brave/build/.ci_features` from the checkout, or, when there is none,
  fetch it from GitHub with a token. In the recipe the checkout always exists by
  the time tests run, so only the local read survives, and with it goes an HTTP
  request and a credential. The gate then interacts with D14 in one place worth
  being deliberate about: a suite skipped by the gate must be recorded as a
  skip, not counted as a missing report, or completeness checking will fail
  every run on which the feature is absent.
- D15 inverts one ordering, and it is worth noticing early. Today
  `report.reportAsanFindings()` opens a Slack thread and passes its id into
  `gh.createTestIssuesOrComments`, so the issues are created knowing where to be
  announced. With issue creation inside the recipe and Slack after it, the
  recipe cannot know a thread that does not exist yet — so the flow reverses:
  the recipe creates the issues and records their URLs, and the Jenkins step
  sends one message containing them. That is simpler than threading a thread id
  through, but it does mean the message shape changes slightly, and the
  interface between the two halves has to be explicit (B4).
- D14 splits reporting three ways, and the split follows from who already holds
  the information. **The harness produces the XML**, by passing
  `--gtest_output=xml:<out>/<builder>/test_results/<binary>.xml` to each binary
  it launches: gtest itself writes the file, so the format is identical to
  today's and the B6 diff stays meaningful, with no report-writing code of our
  own to drift. **The recipe checks completeness**, because only it knows the
  expanded binary list — a suite that dies before writing its report is today
  invisible to `junit`, and becomes an explicit failure. **Jenkins publishes**,
  with one `junit` call over a fixed glob: no `<suite>.txt` index to read, no
  suite list, no `isKnownTestType` assertions, nothing to update when a suite is
  added. The `junit` call itself cannot move — the plugin only ingests during a
  build — but it stops being a place where knowledge lives.
- D20 is the load-bearing choice, and it is worth saying why it beats the
  alternative we had first written down. A schema-with-fields (`gn_args`,
  `gn_imports`, `env_config`, …) filled in per builder puts values in the
  builder, so two builders that want the same thing say it twice and drift
  independently. Upstream's shape puts every value in a _named_ config and
  leaves the builder holding only a list of names — so "what does ASan mean
  here" has exactly one answer, and a second sanitiser builder is a different
  list, not a second copy. That is also what makes the generated files diffable
  in review: a changed config shows up as the same delta in every builder that
  references it.
- Reusing `tools/recipes/config.py` for the resolver is therefore **not** the
  plan (an earlier draft of this document said it was). `config.py` models a
  _schema plus mutating items_, which is a different shape from _named configs
  merged by include order_, and bending one into the other would obscure both —
  and it would also break §7.4, since `config.py`'s idiom has no starlark
  counterpart. The host in `config_lib.py` is a small piece of Python: a DFS
  over an include graph with dict merges, which is most of what `gn_args.star`
  is once lucicfg's graph machinery is taken away. What we do lift from the
  engine's neighbourhood is `freeze`/`thaw` (D21). `config.py` keeps its own
  job: the recipe module's step-level configuration.
- D9 leaves out starlark at no cost in capability, and the one thing starlark
  would have given for free is bought back two ways. It would have _enforced_
  that the spec files are declarative — no imports, no filesystem, and globals
  frozen once a module is loaded. D21 supplies the freezing, and §7.4's
  portability rules plus a lint supply the rest. Neither is as airtight as an
  interpreter that simply cannot do those things; both are cheap, and they keep
  every other part of this in one language.
- D21 is worth doing even setting the port aside. Configuration that is shared
  by reference — one `asan` config read by several builders — is exactly the
  shape where an accidental mutation is both easy and invisible: append to a
  list in one builder's resolution and every other builder that includes it
  silently changes. `freeze` makes that a `TypeError` at the point of the
  mistake. `thaw` exists for the resolver, which legitimately needs mutable
  copies to merge into. Upstream arrived at the same pair for the same reason,
  and lifting theirs keeps the names recognisable: `freeze`, `thaw`,
  `FrozenDict`.
- The recipe module's own configs are a separate thing from all of this, and
  they are still the "first real use of `config.py`" this job was chosen for.
  The split is clean once stated: **`gn_args.py` decides what gets built**, and
  **`recipe_modules/brave_build/config.py` decides how the pipeline behaves** —
  which steps run, what the ASan self-check expects, whether `ptrace_scope`
  needs relaxing. Both are "configs"; only one of them ends up in `args.gn`.
- D13 has one visible cost: the first run of the new job cannot reuse the
  existing `out/Release` cache on the persistent node, so it is a full build.
  That matters only for the B6 comparison, where the first run's wall-clock must
  be discounted rather than read as a regression. The two ASan builders resolve
  to identical GN args, so if cache reuse between them ever matters they can be
  pointed at one shared directory; on separate nodes it makes no difference.
- D12 turns the job's `when { params.SCOPE == … }` branches into a difference
  between two generated `targets.json` files. The two builders reference an
  identical list of GN configs and differ only in their test targets, which is
  the smallest possible demonstration that the config layer works — and the
  reason a third builder (say `linux-x64-ubsan-brave`) should cost a few lines
  of data. The scheduling consequence is that the two nightly timers become two
  job triggers rather than one parameterised trigger, and each builder's
  history, duration and failure rate are then separately visible, which they are
  not today.

## 7. Target architecture

### 7.1 Layers

```
brave/infra/config/                       (new; Track A)
  gn_args.py             the named GN-arg configs — our gn_args.star (D20)
  builders/ci/*.py       builders per group — our chromium.memory.star
      ^ spec: declarative, starlark-portable (§7.4), frozen once read (D21)
  config_lib.py          the host: registration, freeze, resolve, validate
  generate.py            emits the files below; presubmit checks for drift
  generated/builders/<builder>/
      gn-args.json       { args_file, gn_args, secrets } — upstream's shape
      sync.json          target os/arch, gclient overrides, project pins
      targets.json       compile targets, test targets, harness flags
        |
        |  read by both
        v
brave/tools/mb/            (new; Track A)   brave/build/args/*.gni (unchanged, D6)
  mb.py lookup <builder>   print the args.gn it would write   + args/builders/*.gni
  mb.py gen <builder>      write args.gn (+ secrets .gni), run gn gen   (new, thin)
  mb.py validate           every builder resolves; no secret values in git
        ^                                  ^
        |                                  |
  pnpm run build --builder <name>    (D2/D5: dotenv off, buildArgs.ts bypassed;
  pnpm run init|sync --builder <name>       D11: conflicting switches rejected)
        ^
        |
tools/recipes/recipes/browser/build.py            (Track B)
  checkout -> install -> init/sync -> build -> test (runtest.py) -> verify -> report
        |
        v
recipe_modules/{brave_build,brave_test,gh}/       (Track B)
        |
        v
new parallel Jenkins job                          (Track B)
  node lifecycle, credential binding, one engine_bootstrap.py call, teardown
```

### 7.2 The spec files

Two spec files, each the Python analogue of an upstream starlark file, written
so the resemblance is textual and not merely conceptual (D9, §7.4).

`brave/infra/config/gn_args.py` mirrors `infra/config/gn_args/gn_args.star`: one
central file of named configs, the only place a GN arg value is written.

```python
# brave/infra/config/gn_args.py
from config_lib import gn_args

gn_args.config(name = "linux", args = {"target_os": "linux"})
gn_args.config(name = "x64", args = {"target_cpu": "x64"})

gn_args.config(
    name = "release",
    args = {
        "is_debug": False,
        "is_official_build": False,
    },
)

gn_args.config(
    name = "remoteexec",
    args = {
        "use_remoteexec": True,
        "use_siso": True,
    },
)

# Brave's ordered .gni defaults, reduced to one importable file per family so
# that the single-args_file rule holds (see below).
gn_args.config(
    name = "brave_desktop_defaults",
    args_file = "//brave/build/args/builders/desktop.gni",
)

gn_args.config(
    name = "asan",
    configs = ["brave_desktop_defaults"],
    args = {
        "is_asan": True,
        # LSan stays off until the pipeline is stable (D18).
        # https://github.com/brave/brave-browser/issues/56047
        "is_lsan": False,
        "enable_full_stack_frames_for_profiling": True,
        "v8_enable_verify_heap": True,
        # Sanitiser reports are too noisy with DCHECKs on.
        "dcheck_always_on": False,
    },
    secrets = {
        "brave_services_key": "BRAVE_SERVICES_KEY",
        "brave_stats_api_key": "BRAVE_STATS_API_KEY",
        "google_default_client_id": "GOOGLE_OAUTH_CLIENT_ID",
        "google_default_client_secret": "GOOGLE_OAUTH_CLIENT_SECRET",
    },
)
```

`brave/infra/config/builders/ci/sanitizers.py` mirrors
`subprojects/chromium/ci/chromium.memory.star`, down to the group defaults and
the local helper that adds a notification to every builder in the group:

```python
# brave/infra/config/builders/ci/sanitizers.py
from config_lib import builders, gn_args

builders.defaults.set(
    builder_group = "brave.sanitizers",
    execution_timeout_mins = 270,
    channel = "nightly",
    notifies = ["browser-sanitizers-bot"],
)

def sanitiser_builder(*, name, **kwargs):
    kwargs["notifies"] = kwargs.get("notifies", []) + ["browser-bot"]
    return builders.builder(name = name, **kwargs)

sanitiser_builder(
    name = "linux-x64-asan-brave",
    sync_config = builders.sync_config(
        target_os = "linux",
        target_cpu = "x64",
    ),
    gn_args = gn_args.config(
        configs = [
            "release",
            "asan",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    targets = builders.targets(
        compile = ["brave:all"],
        tests = [
            "brave_all_unit_tests",
            "brave_browser_tests",
            "brave_interactive_ui_tests",
            "brave_network_audit_tests",
        ],
    ),
)

sanitiser_builder(
    name = "linux-x64-asan-chromium",
    sync_config = builders.sync_config(
        target_os = "linux",
        target_cpu = "x64",
    ),
    gn_args = gn_args.config(
        configs = [
            "release",
            "asan",
            "remoteexec",
            "linux",
            "x64",
        ],
    ),
    targets = builders.targets(
        compile = ["brave:all"],
        tests = [
            "chromium_unit_tests",
            "browser_tests",
        ],
    ),
)
```

Note the two builders' `gn_args` lists are identical, which is the point: the
difference between them is their `targets`, and "what ASan means" has one
definition. Note also `gn_args.config` used inline, without a `name` —
upstream's `_config` returns an anonymous struct when `name` is unset, and ours
does the same, so a one-off builder need not pollute the config namespace.

### 7.3 The generated file and the secrets `.gni`

Two deliberate divergences from upstream, both in service of Brave's existing
`.gni` layout (D6):

- **One `args_file`, so the imports move into GN.** Upstream permits exactly one
  `args_file` per resolved config, and `buildArgs.ts` currently emits up to four
  ordered imports (`brave_defaults`, `blink_platform_defaults`, a branding file,
  then one of desktop/android/ios). Rather than diverge into an ordered
  `imports` list, add a thin checked-in file per family under
  `brave/build/args/builders/` — `desktop.gni`, `desktop_origin.gni`,
  `android.gni`, `android_origin.gni`, `ios.gni` — each of which does nothing
  but import the existing defaults in the right order. The ordering then lives
  in GN, where it is natural and reviewed once, the single-`args_file` invariant
  holds, and the generated file keeps upstream's shape exactly.
- **A `secrets` map**, which upstream has no need for. It carries GN arg name →
  environment variable name, never a value, and is the only field we add.

The generated result for our builder:

```json
{
  "args_file": "//brave/build/args/builders/desktop.gni",
  "gn_args": {
    "brave_channel": "nightly",
    "dcheck_always_on": false,
    "enable_full_stack_frames_for_profiling": true,
    "is_asan": true,
    "is_debug": false,
    "is_lsan": false,
    "is_official_build": false,
    "target_cpu": "x64",
    "target_os": "linux",
    "use_remoteexec": true,
    "use_siso": true,
    "v8_enable_verify_heap": true
  },
  "secrets": {
    "brave_services_key": "BRAVE_SERVICES_KEY",
    "brave_stats_api_key": "BRAVE_STATS_API_KEY",
    "google_default_client_id": "GOOGLE_OAUTH_CLIENT_ID",
    "google_default_client_secret": "GOOGLE_OAUTH_CLIENT_SECRET"
  }
}
```

`mb gen` renders `args.gn` from it: the `args_file` import, then the secrets
import when `secrets` is non-empty, then the `gn_args` assignments. The secrets
file is written by `mb` from the environment named in `secrets`, with mode
`0600`. A missing secret is a hard error on a builder that declares it, and —
unlike today's `dummy` fallbacks — that failure happens before any compilation
starts. Note that the secrets import is `mb`'s concern, not a config-level one:
keeping it out of the config layer is what preserves the single-`args_file`
rule.

The secrets `.gni` goes **in the output directory**, next to `args.gn`, as
generated per-build state belongs there. Under `--builder` the output directory
is the builder's own `out/<builder>` — `-C` is rejected (D11) — so it is always
inside the source root and always expressible as a source-absolute import:

```gn
# out/linux-x64-asan-brave/args.gn
import("//brave/build/args/builders/desktop.gni")
import("//out/linux-x64-asan-brave/brave_secrets.gni")
target_os = "linux"
is_asan = true
# … the remaining overrides …
```

That gn accepts an `import()` of a `.gni` inside the output directory is
confirmed rather than assumed: a minimal source root with exactly this shape
generates cleanly and the imported value takes effect. Two practical
consequences:

- **No `.gitignore` work.** `out/` is already ignored, which the source-tree
  alternative would not have been.
- **Cleanup is still explicit, and ordered last.** The output directory survives
  between nightly runs on a persistent node, so the file does not disappear on
  its own; it is deleted at the very end of the recipe, after every compile and
  test step, and recreated by the next run's `mb gen`. Deleting it earlier would
  leave `build.ninja` referring to a missing import, so any later bare `ninja`
  would fail its regeneration check. This is the direct replacement for
  `utils.dotEnvCleanup`, with the same "must run even on failure" requirement.

A pleasant property of routing secrets through an import rather than through
`.env`: gn records every file it reads during generation, so a rotated secret
changes the imported file and triggers regeneration, instead of leaving a build
directory quietly configured with a stale value.

Note that `mb lookup` prints only the import line for secrets, so it is safe in
logs; `gn args --list` on such a build directory is **not** — it resolves and
prints the value — and must not be run on CI.

### 7.4 Keeping the spec starlark-portable

Starlark is close enough to a Python subset that a spec of keyword-argument
calls over literals is portable _as text_: the port replaces
`from config_lib import …` with `load(…)`, swaps the host, and leaves the
declarations alone. That only holds if the spec files stay inside the
intersection of the two languages, so the rules are worth writing down and
linting:

| Allowed in a spec file             | Not allowed                                            |
| ---------------------------------- | ------------------------------------------------------ |
| dict, list and string literals     | f-strings (use `%` formatting)                         |
| keyword arguments, trailing commas | classes, decorators, `lambda` beyond trivial use       |
| `def` helpers over the config API  | `while`, recursion, `try`/`except`                     |
| `if` / `for` and comprehensions    | set literals (starlark has no sets)                    |
| `True` / `False` / `None`          | imports other than the single config API line          |
| comments                           | anything touching the filesystem, clock or environment |

Two supporting choices follow from the same goal. The API takes **keyword-only
arguments** (`def config(*, name = None, args = {}, …)`), because starlark's
libraries do and because it reads better anyway. And the host **freezes** every
declaration (D21), so a spec that tries to mutate a config after declaring it
gets a `TypeError` where starlark would have said
`cannot insert into frozen hash table` — the same mistake caught at the same
moment.

What deliberately does _not_ port is the host: registration, resolution, file
writing, and any validation that needs the filesystem. Starlark cannot do I/O at
all, which is why upstream's equivalent lives in Go inside lucicfg. If we ever
move, that is the piece we would hand over, and it is the piece a spec author
never reads.

### 7.5 The Jenkins boundary

The parallel job keeps: node start/check/terminate, workspace selection,
credential binding (`withCreds`, `credentials(...)`), abort-superseded-builds,
build discarder, timers, build naming, Slack messages, S3 upload listing, and
one `junit` call over a fixed glob (D14):

```groovy
junit keepLongStdio: true, skipPublishingChecks: true,
      testResults: "src/out/${BUILDER}/test_results/*.xml"
```

That is the whole of the job's test knowledge: no suite list, no index file, no
per-suite `post` block. Which reports exist, and whether they are all there, is
the recipe's business.

The recipe owns: checkout, dependency install, `init`/`sync`, build
configuration, compile, test execution, report completeness, the sanitiser
self-check, sanitiser finding extraction, and GitHub issue creation.

The recipe learns secrets **only** from the environment Jenkins binds, and never
learns how they are stored.

## 8. Roadmap

Two tracks. Track A is brave-core build tooling and is a prerequisite for most
of Track B; the two can proceed in parallel once A1 has landed, because Track B
can develop against hand-written fixtures of the generated files.

### Track A — build tooling in brave-core

#### A0 — Freeze a behavioural spec and parity fixtures

**Goal.** Know exactly what "the same build" means before changing how it is
produced.

**Work.** From a real nightly run of both scopes, capture the resolved
`args.gn`, the full `.env`, the ordered step list with exact command lines and
per-step environment, the expanded test suite lists from `<out>/<suite>.json`,
and the filter files that applied. Commit them as fixtures.

Record the one intended deviation — sanitiser warnings will start failing runs
that today only get an issue (D17) — so it is not mistaken for a regression, and
record LSan as staying off (D18) so nobody "fixes" it mid-migration. Everything
else is a pure port and any difference is a bug.

Since D17's escalation is independent of the migration, consider landing it in
the current pipeline first, as its own change: the fixtures then already include
it, the new pipeline stays a pure port, and any fallout is attributable to one
commit rather than to a rewritten CI path.

**Exit criteria.** Two consecutive nightly runs produce byte-identical `args.gn`
fixtures. **Size.** Small. **Depends on.** Nothing.

#### A1 — Audit every `.env` consumer; define the builder config schema

**Goal.** Establish that a builder config can replace the dotenv layer, and what
it must contain.

**Work.** Classify every `envConfig.*` call site in `config.ts` (roughly sixty)
into: comes from `package.json` (stays), belongs in the builder config, belongs
on the command line, or is developer-only and simply unavailable under
`--builder`. Method and current classification sketch in Appendix C. Then write
the schema for `gn-args.json`, `sync.json` and `targets.json`, and hand-write
those three files for both `linux-x64-asan-brave` and `linux-x64-asan-chromium`
(D12) so that Track B has something to consume immediately. Writing both, rather
than one, is what proves the schema separates shared configuration from
per-builder differences: the two files should be identical except for
`targets.json`.

This stage also lands the consolidated import files that §7.3 relies on —
`brave/build/args/builders/desktop.gni` and its siblings, each doing nothing but
importing the existing defaults in order — because everything downstream
references one `args_file` and nothing else can be written until they exist.

**Deliverables.** The classification table; the three schemas; the
`build/args/builders/*.gni` files; hand-written `gn-args.json`, `sync.json` and
`targets.json` for both builders, each carrying its `out/<builder>` output
directory (D13). The hand-written `gn-args.json` should already be in the shape
A5's resolver will produce — `{args_file, gn_args, secrets}` — so that A5 is
verified against it rather than defining it.

**Exit criteria.** Every dotenv key the ASan job writes today has a documented
destination, and nothing in the ASan build path depends on a developer-only key.

**Size.** Medium — this is the stage that surfaces surprises. **Depends on.**
A0.

#### A2 — The `mb`-like tool

**Goal.** One tool that turns a builder name into a configured build directory,
usable from both the recipe and a developer's shell.

**Work.** `brave/tools/mb/mb.py` (name to be bikeshedded; it must not be
confused with `src/tools/mb`) with subcommands:

- `lookup <builder>` — resolve and print the exact `args.gn` contents; no side
  effects; secrets shown as the import line only;
- `gen <builder> [--out-dir DIR]` — write `args.gn`, write the secrets `.gni`
  when declared, then run `gn gen`, passing the builder's declared test targets
  (D19) so GN emits their `runtime_deps`. That costs nothing now and is the
  prerequisite for producing isolates later, which is why the plumbing belongs
  here from the start rather than in a retrofit;
- `validate` — every builder resolves, every referenced `.gni` exists, every
  config is reachable, no secret _value_ appears in checked-in data;
- `describe <builder>` / `builders` — for humans and for the recipe.

It reads the generated files only; it knows nothing of configs or how they
resolved. Unit tests compare `lookup` output against the A0 fixtures.

**Exit criteria.** `mb lookup linux-x64-asan-brave` matches the A0 `args.gn`
fixture byte for byte; `mb gen` produces a build directory that `ninja` accepts.

**Size.** Medium. **Depends on.** A1.

#### A3 — Secrets as a generated `.gni`

**Goal.** Remove the last reason for `.env` to exist on CI.

**Work.** Resolve the `secrets` map from the environment; write
`<out>/brave_secrets.gni` with restrictive permissions; fail loudly on a missing
secret; provide explicit cleanup that the recipe calls in a `finally`
equivalent, after every step that compiles or runs anything, plus a
belt-and-braces cleanup in the Jenkins `post` block. No `.gitignore` change is
needed since the file lives under `out/`, but the presubmit should still refuse
a committed `*_secrets.gni` anywhere, in case the pattern gets copied to a
source-tree path later.

**Exit criteria.** A build with real service keys produces the same binaries as
today, with no secret value present in any log or in `args.gn`, and no
`brave_secrets.gni` left in the workspace after the job finishes — including
after a failed job.

**Size.** Small to medium. **Depends on.** A2.

#### A4 — `--builder` in `pnpm run init` / `sync` / `build`

**Goal.** D2, D4, D5 and D11 in force: the same three entry points, driven by a
builder name and by nothing else.

**Work.** Add `--builder <name>` to the commander definitions. When present:
construct `EnvConfig` with the dotenv source disabled; load `sync.json` and
`gn-args.json`; take target OS/architecture, build config, channel and output
directory from the builder config; skip `buildArgs.ts` entirely and delegate
generation to `mb gen` (the existing `use_no_gn_gen` seam is the natural place
to hook in). `init` and `sync` consume `sync.json` for gclient overrides and
project pins.

Then partition every switch of each command into three classes and enforce the
partition in the command's action, before any work starts (D11):

| Class        | Behaviour under `--builder`                                                 | Examples (from the `build` command)                                                                                                                                                                                                                                                                                                                                                                                  |
| ------------ | --------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Owned**    | **Rejected** — hard error, list the flag and say the builder config owns it | `--target_os`, `--target_arch`, `--target_environment`, `-C`, `--channel`, the build-config argument, `--gn`, `--is_asan`, `--is_ubsan`, `--use_clang_coverage`, `--use_remoteexec`, `--offline`, `--skip_signing`, `--notarize`, `--mac_signing_identifier`, `--mac_installer_signing_identifier`, `--mac_signing_keychain`, `--pkcs11-provider`, `--pkcs11-alias`, `--universal`, `--target_android_output_format` |
| **Allowed**  | Passed through — affects how the compile runs, not what is built            | `--target`, `--ninja`, `--force_gn_gen`, `--ignore_compile_failure`, `--prepare_only`                                                                                                                                                                                                                                                                                                                                |
| **Deferred** | Decide per flag during the audit; default to Owned when in doubt            | `--build_omaha`, `--build_sparkle`, `--tag_ap`, `--tag_installdataindex`, `--last_chrome_installer`, `--android_aab_to_apk`, `--android_override_version_name`, `--xcode_gen`                                                                                                                                                                                                                                        |

Defaulting the ambiguous cases to Owned is the safe direction: a rejected flag
produces a loud error that someone fixes in a minute, whereas a silently
honoured one produces a build that does not match its own builder config. Note
that several Owned flags are only meaningful on platforms this pipeline does not
build; they are listed so the rule is uniform rather than linux-specific, since
`--builder` is intended for every platform eventually.

The rejection check belongs with the commander definitions, not inside
`config.ts`, so that it fires before `Config` is constructed and the error names
the flag the user actually typed.

**Exit criteria.** `pnpm run build --builder linux-x64-asan-brave` on a clean
node produces an `args.gn` matching the A0 fixture with no `.env` present;
`pnpm run init --builder …` syncs the same tree as today; and every Owned flag
combined with `--builder` fails with a message naming that flag, covered by a
test per flag class.

**Size.** Medium to large — the blast radius inside `config.ts` is the risk.
**Depends on.** A2, A3.

#### A5 — The spec files, the host, and the generator

**Goal.** Stop hand-writing the generated files; make "what ASan means" exist in
exactly one place; make adding a builder a data-only change; and keep the spec
close enough to upstream's starlark that either a reader or a future port can
move between them without translation.

**Work.** `brave/infra/config/`, split the way upstream splits it — spec on one
side, host on the other:

- **`gn_args.py`** — the analogue of `gn_args/gn_args.star` (D20). Every named
  config: platforms, architectures, `release`, `remoteexec`, the sanitisers, the
  per-family `args_file` configs. The only file in the repository that assigns a
  GN arg value for a builder.
- **`builders/ci/<group>.py`** — analogues of `chromium.memory.star`: group
  defaults, a local helper per group, and builders that are a name plus the
  configs they reference. Our ASan work needs one file with two builders.
- **`config_lib.py`** — the host, and the only place with logic. Registration
  (including `config()` returning an anonymous struct when `name` is unset, as
  upstream's does), `freeze` on everything declared (D21), and the resolver
  reproducing §4.2: memoised depth-first walk, children merged before the node,
  later entries in `configs` overriding earlier, at most one `args_file` per
  resolution, `target_os` and `target_cpu` required, plus a validation hook per
  resolved builder.
- **`generate.py`** — emits `gn-args.json`, `sync.json` and `targets.json` per
  builder plus the locations index. A presubmit re-runs it and fails on drift,
  exactly as upstream's does.

`freeze`/`thaw` themselves are worth lifting properly rather than reinventing:
upstream's live in `recipes-py/recipe_engine/engine_types.py` (`freeze`, `thaw`,
`FrozenDict`), our engine has no equivalent, and both the generator and the
recipe modules want them. Landing them as `tools/recipes/engine_types.py`, under
the same names, keeps the provenance obvious and gives the generator something
to import; if that dependency direction (config generation reaching into the
recipe engine) proves awkward, a shared module is the alternative, but
duplicating the code is not.

Three things to get right, all of which upstream already learned:

- **The derived-args mechanism.** Upstream's `builder_defaults` exists because
  some GN args follow from non-GN-arg builder properties and must still lose to
  an explicit config. We need the same for at least the RBE toggle, hence
  `use_remoteexec`. Copy the precedence, not just the idea: derived args sit
  _below_ everything named.
- **Ordering must be stable.** The output is checked in and reviewed, so
  serialise `gn_args` with sorted keys — which is also what starlark's
  `json.encode` does, so a port would not churn the generated files — and keep
  the include walk deterministic.
- **Portability is a property to maintain, not a one-off.** Apply §7.4's rules
  to `gn_args.py` and the builder files, and add the lint that enforces them.
  Without it the resemblance decays on the first convenient f-string.

**Exit criteria.** Regenerating produces no diff against the hand-written files
from A1; the two ASan builders reference an identical GN config list and differ
only in `targets.json`; changing one config's args changes every referencing
builder's generated file in one commit; a third builder is a data-only change;
two consecutive generations are identical; a spec file that mutates a declared
config fails; and the lint rejects a spec file that steps outside §7.4.

**Size.** Medium. **Depends on.** A1; benefits from A2 being finished so the
consumer is known.

#### A6 — Retire `pnpm run test` for this pipeline

**Goal.** Everything `test.ts` knows lives either in `targets.json` or in the
harness.

**Work.** Port the items in Appendix B: group expansion via
`<out>/<suite>.json`, gn-target-to-executable mapping, filter-file discovery,
the Brave sanitiser environment additions on top of what `runtest.py` already
does, launcher flags, report writing, per-suite default args, the `ci_feature`
gate (D16), and the compile-the-suite step. `pnpm run test` itself stays for
developers until the desktop CI paths no longer use it.

Check `build/.ci_features` before writing the gate: if
`brave_interactive_ui_tests` has become unconditional by then, D16 costs nothing
to drop — one field on a suite entry and the code that reads it.

**Exit criteria.** For each suite of both builders, the harness runs the same
binaries with the same arguments and environment as the A0 fixtures, and
produces XML that Jenkins' `junit` accepts; a gated-off suite is reported as
skipped rather than missing.

**Size.** Large. **Depends on.** A1 (targets schema), A4 (to compile suites).

### Track B — recipes

#### B1 — `brave_build` module and its config

**Goal.** The engine's first production use of `config.py`.

**Work.** `recipe_modules/brave_build/` with `config.py`, `api.py`,
`properties.proto` and examples. The division of labour matters here: anything
the builder config already states is **read**, not restated. So no GN args, no
secret list, no test list — the module config carries only pipeline behaviour:

```python
def BaseConfig(BUILDER='', TARGET_PLATFORM='linux', TARGET_ARCH='x64', **_kw):
    return ConfigGroup(
        builder=Single(str, required=True),   # which generated config to read
        sanitiser=Single(str),                # '', 'asan', 'msan', 'ubsan'
        relax_ptrace_scope=Single(bool),      # the ASan job needs this
        verify_sanitiser=Single(bool),
        # TARGET_* are seeded from the builder's sync.json through
        # get_config_defaults, so items can branch on platform without the
        # recipe threading it through every call.
        TARGET_PLATFORM=Static(TARGET_PLATFORM),
        TARGET_ARCH=Static(TARGET_ARCH),
        BUILDER=Static(BUILDER),
    )
```

with a root item, a mutually exclusive `sanitiser` group, and items for the RBE
toggle. The module owns `pnpm install`, `pnpm run init`/`sync`, and
`pnpm run build --builder`. The output directory comes from the builder config
(D13) rather than the module config, and secret validation reads the `secrets`
map out of `gn-args.json` and checks the environment before the first expensive
step — the module never holds its own copy of either list.

**Exit criteria.** Simulation tests cover each item and each `BadConf` branch,
at 100% coverage; a `set_config('asan')` blob drives a build on a scratch node.

**Size.** Medium. **Depends on.** A1 for the schema, A4 for the build step.

#### B2 — Checkout, install, init and sync

**Goal.** Reach a syncable, buildable tree from the recipe.

**Work.** Extend `brave_core_checkout` with a full (non-sparse) checkout at a
ref, since the module is currently built for sparse toolchain deploys. Add
`pnpm install` honouring the socket-firewall proxy and the existing retries. Add
`init` with the `src/chrome/VERSION` guard, `GIT_CACHE_PATH`, lock cleanup and
the 100-minute timeout, plus the `sync` variant for reused workspaces. No `.env`
writing anywhere.

**Exit criteria.** On a scratch node the recipe reaches a synced tree with no
`.env` present. **Size.** Medium to large. **Depends on.** B1.

#### B3 — `brave_test` module over `runtest.py`

**Goal.** Run the builder's declared test targets, with their per-target data
from `targets.json`.

**Work.** A module that, per test target declared in `targets.json` (D19):
compiles it via `brave_build`, discovers filter files, composes the harness
invocation, runs `runtest.py`, and returns structured results (pass/fail/skip
counts, failed test names with output, sanitiser findings). Before running
anything, compare the declared targets against the gen-time `<out>/<suite>.json`
for every group the builder still compiles, and fail on a difference. The
`verifyAsan` equivalent becomes an explicit step that asserts exactly one
recorded error for `ToolsSanityTest.DISABLED_AddressSanitizerLocalOOBCrashTest`,
with a simulation test proving it fails when the sanitiser is not instrumented.

Report handling per D14. The harness passes
`--gtest_output=xml:<out>/<builder>/test_results/<binary>.xml`, so gtest writes
the report and we own no XML-emitting code. The module then reconciles the
expanded binary list against the files actually present and fails the step for
any binary that produced no report — a case today's `junit` glob cannot
distinguish from a suite with nothing to say. The same structured results feed
B4, so a failure is described once and consumed twice.

Warning escalation per D17. After a binary finishes, scan its output for
`SUMMARY: <name>Sanitizer:` and mark the test that produced it as failed, both
in the structured results and in the XML the JUnit plugin will read. This is
what upstream's `escalate_sanitizer_warnings.py` does to the JSON summary, done
in the one place that already parses that summary. Two details to get right: the
gtest-XML status must be rewritten too, or Jenkins and the recipe will disagree
about the same run; and the ASan self-check deliberately _expects_ a sanitiser
report, so it must be exempt from escalation rather than fighting it.

**Exit criteria.** Both builders' suites run and their results match the A0
fixtures; the self-check fails when `is_asan` is removed from the builder; a
binary killed before it writes its report fails the run rather than passing
quietly; a test that emits a sanitiser report without crashing is reported as
failed by both the recipe and the published XML.

**Size.** Large. **Depends on.** A6, B1.

#### B4 — Reporting in Python

**Goal.** Retire `gh.groovy` and `report.groovy` for this pipeline (D8).

**Work.** A `gh` recipe module that talks to GitHub the way `brockit` does:
subprocess calls to the `gh` command-line tool, through a thin wrapper modelled
on `tools/cr/gh_cli.py`, with one method per `gh` invocation and no policy
inside it. No REST client, and in particular not `script/lib/github.py` — `gh`
already owns authentication, pagination and error reporting, and the Groovy path
we are replacing shells out to the same binary, so behaviour is easier to
compare during the shadow period.

The one adaptation the engine requires is the runner. `gh_cli.py` calls
`terminal.run`; in a recipe every subprocess must go through the `step` seam or
simulation cannot mock it, so the wrapper's `_run` becomes
`api.step([gh, *args], stdout=api.json.output())`. That is worth doing rather
than working around: each `gh` call then appears as a named step with its
arguments in the log, and `api.step_data` seeds its response in tests — which is
the same testing story `gh_cli_test.py` gets from substituting subprocess, only
enforced by the engine. If we later want one wrapper shared between the recipe
and standalone tooling, the runner can be injected instead, leaving argument
construction and JSON parsing in a single file.

Port, from `gh.groovy`: bot label creation, duplicate-issue search by label and
title, comment-on-existing versus create-new, the "previous issues" back-links,
assignee inheritance from closed issues, the fifteen-failure cap, and the
sanitiser-versus-other-failure split. That policy lives in the module's
`api.py`, not in the wrapper. The inputs now come from B3's structured results
rather than the JUnit plugin, and the sanitiser finding comes from the harness
output rather than a console-log scrape.

The known TOCTOU race on issue creation in the Groovy version is worth fixing
during the port rather than reproducing.

Slack stays in Jenkins (D15), so this stage also owes it an interface: a summary
file — suite results, sanitiser findings, and the URL of every issue created or
commented on — written where the job can read it after the recipe exits. Today
those URLs never leave the Groovy process that made them; tomorrow they are the
recipe's output and the Slack step's input.

**Exit criteria.** A dry-run mode reproduces, for a recorded failing build, the
same issue titles, labels and bodies the Groovy path produced; `gh` needs
nothing beyond the `GH_TOKEN` Jenkins already binds; and the summary file
carries every issue URL the Slack message needs.

**Size.** Medium. **Depends on.** B3.

#### B5 — The recipe and the parallel Jenkinsfile

**Goal.** One recipe, two builders (D12), one thin job.

**Work.** `recipes/browser/build.py` with `PROPERTIES` for builder name, ref,
channel and the RBE toggle, and `ENV_PROPERTIES` for the credentials and cache
paths Jenkins binds. The recipe has no notion of scope: it is handed a builder
name, and `linux-x64-asan-brave` versus `linux-x64-asan-chromium` differ only in
the `targets.json` it reads. That removes every `when { params.SCOPE == … }`
branch from the job. The new job definition starts a node, binds credentials,
runs one `engine_bootstrap.py` invocation, publishes the reports with the single
`junit` glob from §7.3 (D14), and tears the node down.

Whether the two builders share one job definition parameterised by builder name
or get a definition each is a Jenkins-side choice with no bearing on the recipe.
A definition each keeps per-builder history and duration readable and matches
the one-timer-per-builder consequence of D12; sharing one definition is less
YAML. Either way the parameter is a builder name, never a scope.

**Exit criteria.** `engine.py test run` green at full coverage; both builders
complete end to end on a real node.

**Size.** Medium. **Depends on.** B3, B4, A5.

#### B6 — Shadow, cut over, decommission

**Work.** Run the new job nightly alongside the old one on the same branch and
channel; compare `args.gn`, step lists, suite pass/fail sets, wall-clock, and
the issues each path would open. Investigate every divergence against the A0
fixtures. After a stable fortnight, point the timers at the new job, keep the
old one dormant for a further period, then prune the ASan-specific Groovy
(`configAsan`, `verifyAsan`, the ASan share of `runTests`) — noting that other
jobs still use most of `utils.groovy`, so this is pruning, not deletion.

**Exit criteria.** Two weeks with no unexplained divergence. **Size.** Medium,
mostly waiting. **Depends on.** B5.

### Suggested landing order

`A0 → A1 → (A2, A5 in parallel) → A3 → A4 → B1 → B2 → A6 → B3 → B4 → B5 → B6`.
A5 can trail A2 as long as the hand-written files from A1 remain the contract.
B1 and B2 can start as soon as A1 fixes the schema, using hand-written generated
files.

## 9. Deliberately deferred

Nothing in Section 6 is left hanging; what follows is out of scope for this
migration but shaped by it, so the plan should not close the door on any of it.

1. **Test isolates, sharding and off-node execution** (D19). The builder
   declares test targets and `mb gen` emits their `runtime_deps`, so producing
   isolates is an added `mb` subcommand rather than a redesign. Everything that
   follows from isolates — sharding, separate tester builders, running tests off
   the build node — needs a scheduler we do not have on Jenkins, so it waits for
   a reason to exist.
2. **A starlark front end** (D9). Python generates the configs. Two things keep
   the door open: the generated JSON is the interface, so no consumer would
   change; and the spec files are written inside the Python/starlark
   intersection (§7.4) with frozen declarations (D21), so the port is a host
   swap rather than a rewrite. lucicfg from depot_tools would be the host — it
   is already installed and can emit arbitrary files — and the cost is then its
   language limits: no recursion, no `while`, module globals frozen after load,
   and no filesystem, so validation that reads files would move to
   `mb validate`.
3. **Slack in Python** (D15). It stays in Jenkins for the cutover, reading the
   recipe's summary file. Moving it later is a self-contained change.
4. **LSan** (D18). Off until the pipeline is stable, then investigated on its
   own (brave-browser#56047) by flipping one builder's data.

## 10. Appendix A — where each piece ends up

| Today                                                       | Destination                                                               |
| ----------------------------------------------------------- | ------------------------------------------------------------------------- |
| `utils.checkoutBraveCore`                                   | `brave_core_checkout`, full-checkout mode (B2)                            |
| `utils.install`                                             | `brave_build.install` (B2)                                                |
| `utils.config`, `configChromium`, `configServiceKeys`       | deleted; `sync.json` + `gn-args.json` + secrets `.gni` (A1, A3)           |
| `utils.configBuildAcceleration`                             | the `remoteexec` config in `gn_args.py` (A5)                              |
| `utils.configAsan`                                          | the `asan` config in `gn_args.py` (A5)                                    |
| `utils.dotEnvCleanup`                                       | secrets `.gni` cleanup (A3)                                               |
| `utils.init`, `runSync`                                     | `brave_build.init` / `.sync` via `--builder` (A4, B2)                     |
| `utils.runBuild`                                            | `brave_build.compile` via `pnpm run build --builder` (A4, B1)             |
| `buildArgs.ts` (for CI)                                     | `mb lookup` / `mb gen` (A2)                                               |
| `test.ts`, `testUtils.js`, `utils.runTests`, `networkAudit` | `targets.json` + `brave_test` over `runtest.py` (A6, B3)                  |
| `utils.verifyAsan`                                          | `brave_test.verify_sanitiser` (B3)                                        |
| `utils.parseTestReports`, `getTestReports`                  | harness writes XML, recipe checks completeness, one `junit` glob (B3, B5) |
| `report.reportAsanFindings`                                 | finding extraction from harness output (B3) + `gh` module (B4)            |
| `gh.createTestIssuesOrComments`, `createIssueOrComment`     | `gh` recipe module over the `gh` CLI (B4)                                 |
| `utils.isCiFeatureSupported`                                | `ci_feature` on a suite entry, read from the checkout (D16, A6)           |
| `nodeManagement.*`, `s3.*`, Slack                           | stay in Jenkins                                                           |

## 11. Appendix B — what retiring `pnpm run test` requires

Each item currently lives in `test.ts` or `testUtils.js` and must reappear in
`targets.json` or in the harness:

- [ ] the run list, now declared per builder as test targets (D19) rather than
      expanded from a group, with `//path:target(toolchain)` still reduced to an
      executable name, and the gen-time `<out>/<suite>.json` compared against
      the declaration as a drift check
- [ ] executable path resolution, including the `bin/run_*` wrappers for the
      Android java/junit suites (not needed for this job, but do not lose it)
- [ ] filter-file discovery: `<suite>`, `<suite>-<platform>`,
      `<suite>-<platform>-<arch>`, `<suite>-<platform>-asan`,
      `<suite>-<platform>-ubsan`, joined with `;`
- [ ] `ASAN_OPTIONS=detect_odr_violation=0` plus `detect_leaks=1` under LSan,
      merged with what `runtest.py` already composes, and set for tests only
- [ ] `LSAN_OPTIONS=suppressions=brave/test/sanitizers/lsan_suppressions.cfg`
- [ ] sanitiser-warning escalation (D17): a `SUMMARY: <name>Sanitizer:` line in
      a test's output fails that test, in the structured results and in the XML;
      new behaviour, not a port — upstream gets it from `testing/test_env.py`
- [ ] `--enable-logging=stderr`, `--v=`, `--test-launcher-jobs`,
      `--test-launcher-bot-mode`, `--gtest_shuffle` for the Brave gtest suites
- [ ] `TEST_PREMATURE_EXIT_FILE`, `LLVM_PROFILE_FILE`, cwd set to the output
      directory
- [ ] report writing, now `--gtest_output=xml:` into one
      `<out>/<builder>/test_results/` directory per D14; the `src/<suite>.txt`
      index is dropped, replaced by the recipe reconciling reports against the
      expanded binary list
- [ ] per-suite default args, notably the two `brave_network_audit_tests`
      timeouts
- [ ] per-suite timeouts (previously `getTestTimeoutMins`)
- [ ] "ignore the test binary's exit code on CI, judge by the report" semantics
- [ ] the `ci_feature` gate (D16): read `build/.ci_features` from the checkout —
      not over HTTP — and record a gated-off suite as skipped, distinctly from a
      suite that produced no report
- [ ] xvfb: `runtest.py` does xvfb and DBus itself, so the `testing/xvfb.py`
      wrapper is dropped rather than ported

## 12. Appendix C — auditing the inputs

Two passes, one per input channel. Anything that resists classification in
either pass is a design question, not a mechanical port, and should be raised
rather than smuggled into the builder config.

**Pass one — every `envConfig.*` call site in `config.ts`** (roughly sixty),
classified as:

- **P** — resolved from `package.json` `config`, unaffected by D2 (Chromium
  repository URL, depot_tools directory and repository, project
  revisions/tags/branches, Brave version);
- **B** — belongs in the builder config (`target_os`, `target_arch`, `is_asan`,
  `use_remoteexec`, `rbe_service`, `rbe_jobs_limit`, `siso_cache_dir`,
  `use_brave_hermetic_toolchain`, `is_brave_release_build`, `brave_channel`,
  gclient custom vars and deps, `cache_dir`, `git_cache_path`);
- **S** — a secret, so a `secrets` entry (`brave_services_key`,
  `brave_stats_api_key`, `brave_google_api_key`, `google_default_client_id`,
  `google_default_client_secret`, per-service keys, and — for other platforms —
  keystore, notary and Sparkle material);
- **D** — developer convenience with no CI meaning (`gclient_verbose`,
  `ignore_patch_version_number`, `brave_extra_gn_gen_opts`, local signing
  identities), simply unavailable under `--builder`.

Note that `gn_args_*` dotenv entries — the `env_gn_args` feature — have no
class: they disappear with the dotenv layer, and D11 gives them no command-line
successor.

**Pass two — every switch of `init`, `sync` and `build`**, classified Owned,
Allowed or Deferred per the table in A4. The two passes must agree: a setting
classified **B** in pass one implies its switch is **Owned** in pass two, and a
switch that stays **Allowed** must not correspond to any **B** entry. That
cross-check is the cheapest way to catch a value that can still be set from two
places.

Anything that resists classification is a design question, not a mechanical
port, and should be raised rather than smuggled into the builder config.

## 13. Appendix D — reference points

- The job:
  `brave-devops/jenkins/jobs/browser/brave-browser-build-linux-x64-asan.yml`
- The shared library: `brave-devops/jenkins/lib/vars/{utils,gh,report}.groovy`
- Build configuration:
  `brave/build/commands/lib/{buildArgs.ts,config.ts,envConfig.ts,build.js}`
- Test knowledge to port: `brave/build/commands/lib/{test.ts,testUtils.js}`,
  `brave/test/filters/`, `brave/test/sanitizers/lsan_suppressions.cfg`
- Engine and config machinery: `brave/tools/recipes/README.md`,
  `tools/recipes/config.py`, `tools/recipes/recipe_modules/hello/config.py`
- `freeze`/`thaw`/`FrozenDict` to lift (D21):
  `chrome_infra/recipes-py/recipe_engine/engine_types.py`
- The composition library §4.2 describes, and the one to mirror:
  `chrome_infra/chromium_infra/starlark-libs/chromium-luci/gn_args.star`
- Sanitiser-warning escalation (D17):
  `src/build/config/sanitizers/sanitizers.gni` (where `fail_on_san_warnings`
  defaults to `using_sanitizer`), `src/testing/test_env.py` and
  `src/tools/memory/sanitizer/escalate_sanitizer_warnings.py`
- Suite group expansion (D19): the `write_file()` calls in `brave/BUILD.gn`
- The `npm` → `pnpm` shim CI still goes through:
  `brave/build/npm_wrapper/npm_wrapper.py`
- Harness already lifted: `tools/recipes/runtest.py`,
  `tools/recipes/gtest_output.py`
- `gh` CLI wrapper to model the reporting port on: `brave/tools/cr/gh_cli.py`,
  with `brave/tools/cr/gh_cli_test.py` for the testing approach
- Chromium's definition:
  `src/infra/config/subprojects/chromium/ci/chromium.memory.star`,
  `src/infra/config/gn_args/gn_args.star`
- Chromium's generated output:
  `src/infra/config/generated/builders/ci/Linux ASan LSan Builder/`,
  `src/infra/config/generated/builders/gn_args_locations.json`
- Chromium's consumption: `src/tools/mb/mb_config.pyl`,
  `chrome_infra/build/recipes/recipe_modules/chromium/{config,api}.py`
