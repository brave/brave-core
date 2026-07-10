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
JSON object whose keys match the recipe's `PROPERTIES`.

To run straight from a pipeline without a checkout, `engine_bootstrap.py`
shallow-clones the engine from brave-core and forwards to `engine.py`:

```sh
curl -sL https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/recipes/engine_bootstrap.py \
    | python3 - toolchains/rust/package_rust \
        --properties '{ "brave_subrevision": 2, "chromium_ref": "151.0.7917.1" }'
```

The engine requires `vpython3` and the bootsrap will take care of these
requirements.

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
registered with `api.post_process(<check>, <args...>)`. A failing check fails
the test; a check may also filter the written expectation. The available checks:

| Check                 | Arguments             | Passes when                                                             |
| --------------------- | --------------------- | ----------------------------------------------------------------------- |
| `MustRun`             | `step_name`           | a step with that exact name ran                                         |
| `DoesNotRun`          | `step_name`           | no step with that name ran                                              |
| `MustRunRE`           | `pattern`             | some step name matches the regex                                        |
| `DoesNotRunRE`        | `pattern`             | no step name matches the regex                                          |
| `StepCommandContains` | `step_name, args`     | the step ran and its command contains `args` as an in-order subsequence |
| `StepCommandRE`       | `step_name, patterns` | the step ran and `patterns[i]` matches `cmd[i]` for each `i`            |
| `StepSuccess`         | `step_name`           | the step ran with retcode `0`                                           |
| `StepFailure`         | `step_name`           | the step ran with a non-zero retcode                                    |
| `StatusSuccess`       | _(none)_              | the overall run status is `SUCCESS`                                     |
| `StatusFailure`       | _(none)_              | the overall run status is `FAILURE` (a checked step failed)             |
| `StatusException`     | _(none)_              | the overall run status is `EXCEPTION` (the recipe raised)               |
| `DropExpectation`     | _(none)_              | always; suppresses writing this test's expectation file                 |

`DropExpectation` is filter-only (it writes/keeps no golden), and should be
considered for cases where there are too many permutations.

Modules are tested via small example recipes under
`recipe_modules/<module>/examples/`, run through this same machinery.
Expectation files sit next to each recipe in a `<recipe>.expected/` directory
and are committed. The `unittests/` suite
(`python3 -m unittest discover -s tools/recipes/unittests -p '*_test.py'`)
covers the machinery itself and, via `recipes_test.py`, fails presubmit if any
expectation is stale.
