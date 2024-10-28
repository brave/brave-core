# Brave NTP

## Overview

Brave NTP is a React application organized into the following folders:

* `components`: The React components implementing the application view layer.
  * Cannot depend upon the application model implementation in `webui`.
* `components/common`: Generically useful helper views.
* `components/context`: React context components.
* `lib`: Utility modules that have no React/view dependencies.
* `models`: Modules that define the application model (state and actions).
  * Modules in this folder cannot have dependencies on modules in other folders.
  * The application model used by the browser is implemented in `webui`, but
  implementations can also be defined for storybook and view testing.
* `stories`: A storybook implementation of the full app.
* `webui`: The implementation of the application model used by the browser.

## Context

Access to the implementation-defined application environment is provided through
two primary context components:

* `AppModelContext`: Provides access to application state and actions.
* `LocaleContext`: Provides access to localized strings.

Context can be accessed through the supporting hooks defined in each context
module.

## Modularity

`AppModel` is composed from a set of supporting modules organized by feature.
Each feature model defines interfaces for the state and actions that it
supports, along with a function returning default state.

Each feature group must provide both a storybook and a browser implementation
defined in `stories` and `webui`, respectively.
