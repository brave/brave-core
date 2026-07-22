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

## Configs

**Configs are a way for a module to expose its "global" state in a reusable,
composable way.** Where [Properties](#properties) are the recipe's _input_,
configs are a module's _tunable internal state_: a module declares the shape of
that state as a schema, then offers named, composable "config items" that a
recipe selects from.

A common problem in building things is an inordinately large matrix of
configuration. A checkout module, for example, has axes like target platform,
target arch, build config, git-cache layout, hermetic-toolchain source, and so
on. There are many combinations but only a relatively small number of _valid_
ones. Configs let a module represent the valid states as named recipes to
follow, rather than leaving every recipe to assemble raw values correctly.

The `hello` recipe module (`recipe_modules/hello/`) is a small, fully worked
example of everything below; the snippets here are drawn from it.

### Declaring a schema

A module opts into the config system by adding a `config.py` next to its
`api.py`. It first defines a _schema_: a callable returning a `ConfigGroup` that
describes the "config blob" the module deals with.

```python
# recipe_modules/hello/config.py
from config import BadConf, ConfigGroup, Single, Static, config_item_context

def BaseConfig(TARGET='Bob'):
    # A config blob is not complete() until every required entry has a value.
    return ConfigGroup(
        verb=Single(str),
        tool=Single(str, required=True),
        # Schema-factory arguments are ALL_CAPS by convention and threaded in as
        # Static (read-only input) data. This avoids name clashes with the
        # per-invocation CONFIG_VARS described below.
        TARGET=Static(str(TARGET)),
    )

config_ctx = config_item_context(BaseConfig)
```

`BaseConfig` must return a `ConfigGroup`; every blob you get from this context
is a modified copy of what the schema returns. The building blocks (all imported
from `config`) are:

| Type          | Holds                                                         |
| ------------- | ------------------------------------------------------------- |
| `Single`      | one scalar of a given type (`required` controls completeness) |
| `Static`      | one immutable, write-once input value (hidden by default)     |
| `Enum`        | one value restricted to a fixed set                           |
| `List`        | an ordered, type-checked list of scalars                      |
| `Set`         | a type-checked set of scalars                                 |
| `Dict`        | a dict with an optional value-type constraint                 |
| `ConfigGroup` | a nested named struct (schemas nest arbitrarily)              |
| `ConfigList`  | an ordered list of `ConfigGroup`s                             |

You manipulate a blob like plain Python data (`c.tool = 'echo'`,
`c.some_set.add(x)`), but assignment is type-checked and the schema is _closed_:
assigning an unknown attribute is an error, and a `Static` member cannot be
reassigned after construction.

`config_ctx` is the context for all config items in this file. The engine
discovers the single `config_item_context(...)` result in a module's `config.py`
and exposes it to the module's `RecipeApi` automatically -- no wiring needed.

### Defining config items

A _config item_ is a function decorated with `config_ctx` that takes a config
blob `c` and mutates it in place (it must not return a value).

```python
# recipe_modules/hello/config.py (continued)

# is_root means every config item applies this one first. At most one per module.
@config_ctx(is_root=True)
def BASE(c):
    if c.TARGET == 'DarthVader':
        c.verb = 'Die in a fire %s!'
    else:
        c.verb = 'Hello %s'

@config_ctx(group='tool')  # Items in the same group are mutually exclusive.
def super_tool(c):
    if c.TARGET != 'Charlie':
        raise BadConf('Can only use super tool for Charlie!')
    c.tool = 'unicorn.py'

@config_ctx(group='tool')
def default_tool(c):
    c.tool = 'echo'
```

The `config_ctx` decorator accepts:

- **`is_root`** -- marks the single "basis" item, applied implicitly before
  every other item on a blob. At most one root per module.
- **`group`** -- items sharing a group are mutually exclusive on one blob;
  applying a second member raises `BadConf`.
- **`includes`** -- names of other config items to run against the blob _before_
  this item's body (already-applied includes are skipped).
- **`deps`** -- group names that must already be satisfied on the blob before
  this item may apply; otherwise `BadConf`.

Any violation (double application, group conflict, unmet `deps`, a failing
`include`) raises `BadConf`.

### Using a config

`RecipeApi` provides all the plumbing. A module reads its current blob as
`self.c`, and a recipe reaches it directly as `api.<module>.c`.

```python
# recipe_modules/hello/api.py
from recipe_api import RecipeApi

class HelloApi(RecipeApi):
    def greet(self):
        self.m.step('Greet Admired Individual', [
            self.m.path.workspace / self.c.tool,
            self.c.verb % self.c.TARGET,
        ])
```

A recipe (or another module) selects a config with `set_config`:

```python
# recipe_modules/hello/examples/simple.py
DEPS = ['hello']

def RunSteps(api):
    api.hello.set_config('default_tool')
    api.hello.greet()  # Greets 'Bob' with echo.
```

`set_config(name, **CONFIG_VARS)` builds a fresh blob and assigns it to
`api.hello.c`. It does so by:

1. computing the schema arguments (`CONFIG_VARS`), lowest-to-highest precedence:
   `get_config_defaults()` (overridable on the api to compute defaults
   dynamically), then the keyword arguments to `set_config`;
2. instantiating the schema with those arguments (`BaseConfig(**CONFIG_VARS)`);
3. applying the named config item -- and its root and `includes` -- to the blob.

So passing a `CONFIG_VARS` value steers both the schema and the items that
branch on it:

```python
api.hello.set_config('super_tool', TARGET='Charlie')  # -> unicorn.py, 'Hello Charlie'
api.hello.set_config('default_tool', TARGET='DarthVader')  # -> echo, 'Die in a fire ...'
```

Two lower-level entry points are also available, though `set_config` is
preferred:

- **`make_config(name,
  **CONFIG_VARS)`** returns a blob without storing it in `self.c`.
- **`apply_config(name)`** applies an additional named item on top of the
  existing `self.c`, layering configs after an initial `set_config`.

> **Note:** older versions of chrome-infra's `set_config` also applied the named
> item across the module's `DEPS`. That behaviour was
> [recognized as a design mistake](https://chromium.googlesource.com/infra/luci/recipes-py/)
> and removed upstream; this engine matches current upstream and does **not** do
> it. `set_config` only ever touches the current module's own `self.c`.

### Testing configs

Config code is subject to the same 100% coverage requirement as the rest of a
module (see [Testing](#testing)), so exercise each item and each branch from
example/test recipes. A config item that raises `BadConf` surfaces as an
`EXCEPTION`-status run, which a `tests/` recipe can assert:

```python
# recipe_modules/hello/tests/badconf.py
from post_process import DropExpectation, StatusException

DEPS = ['hello']

def RunSteps(api):
    api.hello.set_config('super_tool', TARGET='Not Charlie')  # raises BadConf

def GenTests(api):
    yield api.test(
        'badconf',
        api.post_process(StatusException),
        api.post_process(DropExpectation),
        status='EXCEPTION',
    )
```

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
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
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
