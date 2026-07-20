# Brave recipes

> [!WARNING]
>
> Highly experimental. The engine, modules, and recipes here are a work in
> progress and may change or be removed without notice.

A very small recipe engine for Brave, loosely modelled on
[chrome-infra's recipes_py](https://chromium.googlesource.com/infra/luci/recipes-py/).
A _recipe_ describes a pipeline; the _engine_ resolves the recipe's `DEPS`,
instantiates each _recipe module_, and runs the recipe's `RunSteps`.

## Usage

```sh
vpython3 tools/recipes/engine.py toolchains/rust/package_rust \
    --properties '{ "brave_subrevision": 2, "chromium_ref": "151.0.7917.1" }'
```

The recipe name is a `/`-separated path under `recipes/`. `--properties` is a
JSON object decoded into the recipe's `PROPERTIES` message (see
[Properties](#properties)).

To run straight from a pipeline without a checkout, `engine_bootstrap.py`
shallow-clones the engine from brave-core and forwards to `engine.py`:

```sh
curl -sL https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/recipes/engine_bootstrap.py \
    | python3 - toolchains/rust/package_rust \
        --properties '{ "brave_subrevision": 2, "chromium_ref": "151.0.7917.1" }'
```

The engine requires `vpython3` and the bootsrap will take care of these
requirements.

## Properties

A recipe observes its input by defining protobuf messages and setting them as
its `PROPERTIES` and (optionally) `ENV_PROPERTIES`. Declare the messages in a
sibling `.proto` file:

```proto
// recipes/toolchains/rust/package_rust.proto
syntax = "proto3";
package recipes.brave.toolchains.rust.package_rust;

message InputProperties {
  int32 brave_subrevision = 1;
  string chromium_ref = 2;
}

message EnvProperties {
  string GIT_CACHE = 1;  // All envvar keys must be capitalized.
}
```

Then import the generated messages from the `PB` namespace and assign them:

```python
# recipes/toolchains/rust/package_rust.py
from PB.recipes.brave.toolchains.rust.package_rust import (EnvProperties,
                                                           InputProperties)

PROPERTIES = InputProperties
ENV_PROPERTIES = EnvProperties

def RunSteps(api, properties, env_properties):
    # properties and env_properties are instances of their respective proto
    # messages.
    api.chromium_checkout.ensure_checkout(
        ref=properties.chromium_ref,
        git_cache=env_properties.GIT_CACHE or None)
```

`RunSteps` takes `api` followed by whichever of the two messages the recipe
declares:

```python
def RunSteps(api):                               # neither declared
def RunSteps(api, properties):                   # PROPERTIES only
def RunSteps(api, properties, env_properties):   # PROPERTIES and ENV_PROPERTIES
def RunSteps(api, env_properties):               # ENV_PROPERTIES only
```

`PROPERTIES` is populated by taking the input property JSON object (from
`--properties`), removing all keys beginning with `$`, and decoding the rest as
JSONPB into the `PROPERTIES` message. Keys beginning with `$` are reserved by
the engine.

`ENV_PROPERTIES` is populated by taking the current environment variables,
capitalizing all keys (`key.upper()`), and decoding that into the
`ENV_PROPERTIES` message. Both decodes ignore unknown fields.

Properties are set in tests via the same messages (see [Testing](#testing)):

```python
def GenTests(api):
    yield api.test(
        'example',
        api.properties(InputProperties(chromium_ref='151.0.7917.1',
                                       brave_subrevision=1)),
        api.properties.environ(EnvProperties(GIT_CACHE='/b/cache')),
    )
```

`api.properties` also accepts top-level keyword arguments as a shorthand, e.g.
`api.properties(chromium_ref='151.0.7917.1', brave_subrevision=1)`.

## Testing

Recipes and recipe modules are tested by _simulation_: a recipe declares
`GenTests(api)` yielding one case per run, the engine runs `RunSteps` with every
side effect mocked (no subprocess, no real filesystem or environment), and the
recorded step stream is checked against a committed JSON _expectation_.

```sh
vpython3 tools/recipes/engine.py test run      # compare against expectations
vpython3 tools/recipes/engine.py test train    # (re)write expectations
vpython3 tools/recipes/engine.py test list     # list all test case ids
vpython3 tools/recipes/engine.py test run --filter package_rust
```

On a full (unfiltered) `run` or `train`, the runner also enforces 100% line
coverage of all `recipes/` and `recipe_modules/` source, reports any module with
no test coverage at all, and (on `run`) flags orphaned expectation files -- any
of which fails the run. Production-only I/O backends (the `_Real*` seams, never
exercised by simulation) are marked `# pragma: no cover`.

Simulation is possible because the seam modules, such as `step`, `path`, `env`,
`platform`, are the only ones that touch the outside world, and in test mode
they read/mutate the case's simulated state instead. A recipe or module that
does I/O must go through them (e.g. `api.path.exists(...)`,
`api.env.which(...)`) rather than `os`/`pathlib`/`shutil` directly.

A test case is built from fragments merged by `api.test`:

```python
import post_process

def GenTests(api):
    yield api.test(
        'linux',
        api.chromium_checkout.with_git_cache(),                 # seed preconditions
        api.brave_core_shallow.deployed('tools/cr/toolchains'),
        api.properties(brave_subrevision=1, chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'fetch chromium'),
        api.post_process(post_process.StatusSuccess),
    )
```

Fragment builders live on the root api (`api.test`, `api.properties`,
`api.step_data`, `api.post_process`) and on each DEPS module's `TEST_API`
(`api.platform.name('mac')`, `api.path.files(...)`, `api.env.set(...)`, and
module-specific precondition helpers).

`post_process` checks run against the recorded steps after simulation, each
registered with `api.post_process(<check>, <args...>)`. A failing check does not
abort its hook: it is recorded with the failing source line and the resolved
values of its sub-expressions (AST-introspected), and fails the test. A check
may also return a filtered `steps` mapping to narrow the written expectation.
The available checks:

| Check                 | Arguments             | Passes when                                                                        |
| --------------------- | --------------------- | ---------------------------------------------------------------------------------- |
| `MustRun`             | `step_name...`        | every named step ran                                                               |
| `DoesNotRun`          | `step_name...`        | none of the named steps ran                                                        |
| `MustRunRE`           | `pattern`             | some step name matches the regex                                                   |
| `DoesNotRunRE`        | `pattern...`          | no step name matches any regex                                                     |
| `StepCommandContains` | `step_name, args`     | the step ran and its command contains `args` as an in-order subsequence            |
| `StepCommandRE`       | `step_name, patterns` | the step ran, each `patterns[i]` fully matches `cmd[i]`, with no surplus of either |
| `StepSuccess`         | `step_name`           | the step ran with retcode `0`                                                      |
| `StepFailure`         | `step_name`           | the step ran with a non-zero retcode                                               |
| `StatusSuccess`       | _(none)_              | the run succeeded (`$result` has no `failure`)                                     |
| `StatusFailure`       | _(none)_              | the run had a non-infra failure (a checked step failed)                            |
| `StatusException`     | _(none)_              | the run had an infra failure (the recipe raised)                                   |
| `StatusAnyFailure`    | _(none)_              | the run failed, infra or non-infra                                                 |
| `DropExpectation`     | _(none)_              | always; suppresses writing this test's expectation file                            |

`DropExpectation` is filter-only (it writes/keeps no golden), and should be
considered for cases where there are too many permutations.

Modules are tested via small example recipes under
`recipe_modules/<module>/examples/` (with focused edge cases under
`recipe_modules/<module>/tests/`), run through this same machinery. Expectation
files sit next to each recipe in a `<recipe>.expected/` directory and are
committed.
