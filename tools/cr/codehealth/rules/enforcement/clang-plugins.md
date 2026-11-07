---
name: enforcement/clang-plugins
rubric: enforcement
severity: deprecation
---

**Rule.** Brave code complies with the clang plugin checks that upstream
enforces, or is about to enforce, in its directories.

**Why.** New plugin flags signal per-directory enforcement, which reaches
Brave's code too: path matching is by substring, so
`brave/third_party/blink/...` is covered by a `third_party/blink/` entry. The
checks are off in official builds, so a release build doesn't catch them.

**How to check.** What's enforced today is in
`build/config/raw_ptr_plugin_config.yaml`; where it's heading is in
`tools/clang/raw_ptr_plugin/PluginConfig.h` `Default()`. For each candidate
commit, read the flags or paths it adds, and grep Brave's sources under the
paths they now cover for the pattern the check rejects.
