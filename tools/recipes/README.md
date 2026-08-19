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
```

Then import the generated message from the `PB` namespace and assign it:

```python
# recipes/toolchains/rust/package_rust.py
from PB.recipes.brave.toolchains.rust.package_rust import InputProperties

PROPERTIES = InputProperties

def RunSteps(api, properties):
    # properties is an instance of the proto message above.
    api.chromium_checkout.ensure_checkout(ref=properties.chromium_ref)
```

An `ENV_PROPERTIES` is very similar to a regular property. An example of a an
`ENV_PROPERTIES` being used can be found in the `git_cache` recipe module.

```proto
// recipe_modules/git_cache/properties.proto
syntax = "proto3";
package recipe_modules.brave.git_cache;

message EnvProperties {
  string GIT_CACHE_PATH = 1;  // All envvar keys must be capitalized.
}
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
JSONPB into the `PROPERTIES` message. Keys beginning with `$` are reserved: a
`$<module>` key carries that module's own properties (see
[Per-module properties](#per-module-properties)).

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
        api.properties.environ(GIT_CACHE_PATH='/b/cache'),
    )
```

`api.properties` also accepts top-level keyword arguments as a shorthand, e.g.
`api.properties(chromium_ref='151.0.7917.1', brave_subrevision=1)`.

### Per-module properties

A recipe _module_ can declare its own `PROPERTIES` (and optionally
`ENV_PROPERTIES`) too, so it observes typed input without threading it through
every recipe. Declare the messages in a sibling `.proto`, with the package set
to the module's namespace (the file name is dropped -- the single-repo analogue
of upstream's `$repo/module`):

```proto
// recipe_modules/hello/properties.proto
syntax = "proto3";
package recipe_modules.brave.hello;

message InputProperties {
  string target = 1;
}
```

Assign the message as `PROPERTIES` in the module's `__init__.py` (next to
`DEPS`) and accept it in the api's constructor:

```python
# recipe_modules/hello/__init__.py
from PB.recipe_modules.brave.hello.properties import InputProperties

DEPS = ['path', 'step']
PROPERTIES = InputProperties
```

```python
# recipe_modules/hello/api.py
class HelloApi(RecipeApi):
    def __init__(self, properties):
        super().__init__()
        # DEPS are NOT available yet in __init__ -- only stash the value here.
        self._target = properties.target or None
```

The engine passes the bound message(s) positionally, in the same order
`RunSteps` receives them:

```python
def __init__(self):                            # neither declared
def __init__(self, properties):                # PROPERTIES only
def __init__(self, properties, env_properties): # PROPERTIES and ENV_PROPERTIES
def __init__(self, env_properties):            # ENV_PROPERTIES only
```

A module's `PROPERTIES` are **namespaced**: they are decoded from the
`$<module_name>` block of the input property JSON, keeping per-module input
separate from the recipe's own top-level properties. So to greet `anya` via the
`hello` module above:

```sh
vpython3 tools/recipes/engine.py some/recipe \
    --properties '{ "$hello": { "target": "anya" } }'
```

and in tests:

```python
api.properties(**{'$hello': {'target': 'anya'}})
```

`ENV_PROPERTIES` on a module works exactly as it does for a recipe (from the
environment, keys upper-cased). Both decodes ignore unknown fields, and (as with
recipes) there is no "required" enforcement -- a missing property takes its
proto default. Per-module properties pair naturally with [Configs](#configs): a
common pattern is for `get_config_defaults` to feed a property into the config
schema (the `hello` module does this with `target` -> `TARGET`).

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

## Getting data back from a step

Consider this recipe:

```python
# recipes/shake.py
import post_process

DEPS = ['path', 'step']

def RunSteps(api):
    result = api.step('determine blue moon',
                      [api.path.workspace / 'is_blue_moon.sh'],
                      check=False)
    if result.retcode == 0:
        api.step('HARLEM SHAKE!',
                 [api.path.workspace / 'do_the_harlem_shake.sh'])
    else:
        api.step('boring', [api.path.workspace / 'its_a_small_world.sh'])

def GenTests(api):
    yield api.test(
        'harlem',
        api.step_data('determine blue moon', retcode=0),
        api.post_process(post_process.MustRun, 'HARLEM SHAKE!'),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'boring',
        api.step_data('determine blue moon', retcode=1),
        api.post_process(post_process.MustRun, 'boring'),
        api.post_process(post_process.DropExpectation),
    )
```

The `check=False` is what makes reacting to a retcode possible at all: by
default any non-zero exit raises `subprocess.CalledProcessError` and the recipe
is over.

See how `result` carries the outcome of the step that just ran? What you get
back is a `step_data.StepData`, and these members are always there:

- **`name`** -- the step's name.
- **`retcode`** -- pretty much what you think.
- **`stdout`** / **`stderr`** -- the result of the step's `stdout`/`stderr`
  placeholder, or `None` when the step didn't redirect that handle (see below).

This is pretty neat... but it turns out retcodes are a miserable way to
communicate anything more than "did it work". `api.json.output()` to the rescue:

```python
# recipes/war.py
import post_process

DEPS = ['json', 'path', 'step']

def RunSteps(api):
    result = api.step(
        'run tests',
        [api.path.workspace / 'do_test_things.sh', api.json.output()])
    num_passed = result.json.output['num_passed']
    if num_passed > 500:
        api.step('victory', [api.path.workspace / 'do_a_dance.sh'])
    elif num_passed > 200:
        api.step('not defeated', [api.path.workspace / 'woohoo.sh'])
    else:
        api.step('deads!', [api.path.workspace / 'you_r_deads.sh'])

def GenTests(api):
    for name, passed in (('winning', 791), ('not_dead_yet', 302),
                         ('noooooo', 10)):
        yield api.test(
            name,
            api.step_data('run tests', api.json.output({'num_passed':
                                                        passed})),
            api.post_process(post_process.StatusSuccess),
            api.post_process(post_process.DropExpectation),
        )
```

### How does THAT work!?

`api.json.output()` returns a [placeholder](#placeholders), which is meant to be
dropped into a step's command list. When the step runs, the placeholder is
_rendered_ into some strings (here, something like `/tmp/some392ra8`). When the
step finishes, the placeholder reads that file back and adds the parsed data to
the step's `StepData`, filed under the module and method that produced it -- so
the `json` module's `output` method lands at `result.json.output`. Do take a
peek at `recipe_modules/json/api.py`; it is short.

### Example: read a step's standard output as JSON

```python
result = api.step(..., stdout=api.json.output())
# result.stdout is a parsed JSON value, such as a dict.
```

### Example: read a step's standard output as text

```python
result = api.step(..., stdout=api.raw_io.output_text())
mirror_dir = result.stdout.strip()
```

Also see [`raw_io`'s example](recipe_modules/raw_io/examples/full.py).

### Example: write to a step's standard input

```python
api.step(..., stdin=api.raw_io.input_text('test input'))
```

### Example: write to a step's standard input as JSON

```python
api.step(..., stdin=api.json.input({'value': 1}))
```

Also see [`json`'s example](recipe_modules/json/examples/full.py).

### Example: several outputs from one step

Give each placeholder a `name` and they are told apart on the result, under the
plural of the method that made them:

```python
result = api.step('run tests', [
    script,
    api.json.output(name='fast'),
    api.json.output(name='slow'),
])
result.json.outputs['fast']  # and ['slow']
```

### Example: simulated step output

This specifies the output a step should be seen to produce when it runs in
simulation. It is the usual way to keep test data next to the code that knows
what the step does, instead of spelling it out in every `GenTests` case:

```python
api.step(..., step_test_data=lambda: api.raw_io.test_api.stream_output_text(
    'test data'))
```

A step's own default is _merged under_ whatever a test seeds for it, so a test
can leave it alone, add to it, or replace it outright with
`api.override_step_data`.

### Example: simulated step output for a test case

```python
yield api.test(
    'my_test',
    api.step_data('step_name', stdout=api.raw_io.output_text('test data')),
)
```

### Example: a whole directory of output

When a step produces more than one file, `api.raw_io.output_dir()` hands it a
directory and gives you back a mapping of relative path to content. The files
are read lazily, so a step is free to leave more behind than the recipe looks
at:

```python
result = api.step('dump', ['dump_files', api.raw_io.output_dir()])
outdir = result.raw_io.output_dir

set(outdir)                # every path the step wrote
outdir['some/file']        # read now, cached from here on
del outdir['some/file']    # hand that memory back
```

Seed it in a test with the files the step should be seen to have written:

```python
api.step_data('dump', api.raw_io.output_dir({'some/file': b'contents'})),
```

The directory itself comes from `api.path.mkdtemp()`, under
`api.path.cleanup_dir` (`<workspace>/rc`) -- treat everything there as
disposable. Pass `leak_to` to use a directory of your own choosing instead.

### Example: leaving the file behind

An output placeholder normally writes to a temporary file that is deleted once
the step is done. Pass `leak_to` and it writes to a path you choose and stays
there, so a later step can pick it up:

```python
config = api.path.out / 'config.json'
api.step('write config', ['generate', api.json.output(leak_to=config)])
api.step('use config', ['build', '--config', config])
```

## <a name="placeholders"></a>What are placeholders and how do they work?

Placeholders are wrappers around the inputs and outputs of recipe steps. They
give you somewhere to put data-processing (JSON parsing, proto decoding) and,
just as importantly, a seam to mock in tests.

### Example

```python
result = api.step('run a cool script',
                  ['really_cool_script.py', '--json-output-file',
                   api.json.output()],
                  check=False)
print(result.json.output)
```

There is a fair bit going on under those two lines. Let's dig in.

`api.json.output()` returns a `JsonOutputPlaceholder`, a subclass of
`recipe_api.OutputPlaceholder`, which has two methods that matter here:
`render()` and `result()`. The engine replaces each placeholder in the command
list with whatever `render()` returns. `JsonOutputPlaceholder` picks a file name
and returns it; say `render()` gives us `/tmp/output.json`.

So what actually gets executed is:

```sh
really_cool_script.py --json-output-file /tmp/output.json
```

When the program returns, the engine calls `JsonOutputPlaceholder.result()` and
files what it hands back into `result.json.output`. Here `json` is the name of
the recipe module and `output` is the name of the method that returned the
placeholder. `JsonOutputPlaceholder.result()`, for its part, parses the JSON out
of `/tmp/output.json`.

Where a result is filed depends on whether the placeholder was given a `name`:

| Placeholders on the step               | Where their results land                                      |
| -------------------------------------- | ------------------------------------------------------------- |
| `api.json.output()`                    | `result.json.output`                                          |
| `api.json.output(name='cfg')`          | `result.json.outputs['cfg']`, and also `result.json.output`   |
| `output(name='a')`, `output(name='b')` | `result.json.outputs['a']` and `['b']` -- there is no default |
| two unnamed `api.json.output()`        | an error: nothing tells them apart                            |

A placeholder passed as a step's `stdin`/`stdout`/`stderr` is not in the command
list, so there is nothing to render into it; the engine redirects the handle at
the placeholder's file instead, and an output placeholder's result becomes
`result.stdout`/`result.stderr`. A placeholder that doesn't stand for a single
file -- `api.raw_io.output_dir()` -- says so with `is_file_backed = False`, and
is rejected on a std handle.

### Tests and mocks

```python
yield api.test(
    'test really_cool_script.py',
    api.step_data('run a cool script', api.json.output({'json': 'object'})),
)
```

This test case stubs out the actual invocation of `really_cool_script.py` and
feeds that dictionary straight into `result.json.output`.

Behind the scenes this works because the `json` module also has a `test_api.py`
with an `output` method on it. The `api.json.output` in `GenTests` is a
different function from the `api.json.output` the recipe calls -- one seeds the
data, the other creates the placeholder that hands it back -- and they find each
other because both are registered under the same module and method name (see
`recipe_api.returns_placeholder` and `recipe_test_api.placeholder_step_data`).

### The modules that provide placeholders

| Module   | Input                                | Output                                                            |
| -------- | ------------------------------------ | ----------------------------------------------------------------- |
| `raw_io` | `input` (bytes), `input_text`        | `output` (bytes), `output_text`, `output_dir` (a whole directory) |
| `json`   | `input`                              | `output` (parsed with `json.loads`)                               |
| `proto`  | `input` (`BINARY`/`JSONPB`/`TEXTPB`) | `output` (decoded into a message class)                           |

`raw_io` is the bottom of the stack -- the other two are it wearing a codec --
so a new one is mostly a `render`/`result` pair delegating to a `raw_io`
placeholder. The `file` module is a worked example of using them: reading a file
is a step that copies it _into_ an output placeholder, and writing one is a step
that copies _from_ an input placeholder.

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
`api.step_data`, `api.override_step_data`, `api.post_process`) and on each DEPS
module's `TEST_API` (`api.platform.name('mac')`, `api.path.files(...)`,
`api.env.set(...)`, and module-specific precondition helpers).

`api.step_data(name, ...)` is how a step's simulated result is seeded: a
retcode, the data its output placeholders hand back, and what it wrote on
`stdout`/`stderr`. See
[Getting data back from a step](#getting-data-back-from-a-step) for the whole
picture; the short version is:

```python
api.step_data('flaky', retcode=1),
api.step_data('run tests', api.json.output({'num_passed': 791})),
api.step_data('git cache exists', stdout=api.raw_io.output_text('/b/cache/x')),
```

A step can also carry its own default simulated result via `step_test_data=`, so
the common case needs no per-test seeding at all; `api.override_step_data`
replaces that default rather than merging into it.

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
