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
python3 tools/recipes/engine.py toolchains/rust/package_rust \
    --properties '{ "chromium_src": "~/dev/brave-next/src/", "brave_subrevision": 2, "chromium_ref": "151.0.7917.1" }'
```

The recipe name is a `/`-separated path under `recipes/`. `--properties` is a
JSON object whose keys match the recipe's `PROPERTIES`.

To run straight from a pipeline without a checkout, `engine_bootstrap.py`
shallow-clones the engine from brave-core and forwards to `engine.py`:

```sh
curl -sL https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/recipes/engine_bootstrap.py \
    | python3 - toolchains/rust/package_rust \
        --properties '{ "chromium_src": "~/dev/brave-next/src/", "brave_subrevision": 2, "chromium_ref": "151.0.7917.1" }'
```
