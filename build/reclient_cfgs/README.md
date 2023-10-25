This directory contains configuration script and reclient configs to support
compilation on linux remotes.

### Configuration steps of `configure_reclient.py`

1. Generate reproxy and rewrapper configs.
2. Generate `python_remote_wrapper`.
3. If a host machine is not linux, download linux toolchain and generate
   `clang_remote_wrapper`.

##### Reproxy config merge

Reproxy config is based on the Chromium config template.
1. Load and substitute
   `//buildtools/reclient_cfgs/reproxy_cfg_templates/reproxy.cfg.template` with
   empty variables. This is done to detect new template variables in the config.
2. Merge `//brave/build/reclient_cfgs/reproxy.cfg`.
3. Set auth-specific vars from `RBE_*` environment variables if they are set.

##### Rewrapper configs merge

Rewrapper configs are based on the Chromium linux config.
1. Load `//buildtools/reclient_cfgs/linux/<tool>/rewrapper_linux.cfg`
2. Merge `//brave/build/reclient_cfgs/<tool>/rewrapper_base.cfg`
3. Merge `//brave/build/reclient_cfgs/<tool>/rewrapper_<host>.cfg`

##### Remote wrappers

`python_remote_wrapper` adds `PYTHONPATH` into envrionment and runs the passed
command as is.

`clang_remote_wrapper` is required to run cross-compilation. It replaces default
clang path with a linux clang path and runs the linux version of the clang
remotely.

### Config merge process

1. Parse a config item and merge it with the existing one or create a new one.
2. If the value is a map, overwrite map items; if it's a list, append
   list items. Map-like and list-like items are hardcoded in
   `from_reclient_cfg_value()`.
3. If the value is empty, clear the item.

This allows config merger to perform such modifications:

```
# Add/modify/remove items into a map-like value.
# This adds/modifies "OSFamily" and removes "label:action_default".
platform=OSFamily=linux,label:action_default=

# Add items into a list-like value.
inputs=src/new_file,src/new_file2

# Modify a simple value.
canonicalize_working_dir=false

# Fully replace list/map values by clearing it first.
labels=
labels=type=compile,compiler=clang,lang=cpp
platform=
platform=container-image=docker://...
inputs=
inputs=src/a_single_input
```
