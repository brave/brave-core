# Brave docs

This directory contains documentation for the Brave Browser. For additional resources outside this repository, go to:

 * [brave-browser/wiki](https://github.com/brave/brave-browser/wiki)

## Document Index

### Checking Out and Patching
*   [Upgrading Chromium](chromium_version_upgrade.md) Upgrading `brave-core` to
    a newer Chromium version.
*   [Patching with `plaster`](plaster.md) A patching engine for semantical
    patching of upstream Chromium.
*   [WebUI Overriding](webui_overriding.md)
*   [DevTools Frontend Patching](devtools_frontend_patching.md) Managing
    changes to the upstream project.

### Tools
*   [Claude Code Skills](claude_code_skills.md) - Slash commands for automating
    common development tasks (commit, review, preflight, CI, and more).

### Security and Privacy
*   [Premium Account Privacy](premium_account_privacy.md) -
    How blind tokens decouple payment identity from service usage.
*   [Cryptographic Features](cryptography.md) - Cryptographic features in Brave
    beyond standard Chromium encryption.

### General Development
*   [Running test suites](running_test_suites.md) - Selectively execute unit,
    browser and typescript tests.
*   [`gni` notes](gni_sources.md) - Brief notes on the use of `source.gni`
    files in our code base.
*   [Rust notes](rust.md) - General recommendations on integrating rust code
    into `brave-core`
*   [Adapting Chromium tests to the Brave Codebase](adapting_chromium_tests.md) -
    Suggestions on how to make chromium test work on our test targets.
*   [Ship a File to All Clients](ship_a_file_to_all_clients.md) - How to ship a
    file to all clients via component updater.
*   [Git Configuration](git_configuration.md) - General recommendations for
    setting up and optimizing your Git environment.
*   [Ignoring Files from Format and Presubmit Checks](ignoring_format_checks.md) -
    How to exclude files from format and presubmit checks.
*   [Siso Customization](siso_customization.md) - How we customize Siso to work
    for us.
*   [WebUI Frontend](./webui_frontend.md) - General Web UI, React, webpack
    overview.
*   [WebUI Testing](./webui_testing.md) - Guide to creating and running *.test.ts(x) suites.
*   [WebUI Strings Overview](./webui_strings_explainer.md) - Including strings in a Web UI frontend.


## Creating Documentation

> [!IMPORTANT]
> If you add new documents, please also add a link to them in the Document
> Index below.

### Guidelines

*   Markdown documents must follow the
    [Markdown Style
    Guide](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/markdown/markdown.md).

### Previewing changes

#### Locally using [md_browser](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/tools/md_browser/)

```bash
# in src/brave/
npm run docs
```

This is only an estimate. The **github** view may differ.
