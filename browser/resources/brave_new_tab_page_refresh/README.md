# Brave NTP

## Overview

Brave NTP is a React application organized into the following folders:

* `api`: Modules that define the application model (state and actions).
  * Modules in this folder cannot have dependencies on modules in other folders.
  * The application APIs used by the browser are implemented in `webui`, but
  implementations can also be defined for storybook and view testing.
* `components`: The React components implementing the application view layer.
  * Cannot depend upon the application API implementations in `webui`.
* `components/common`: Generically useful helper views.
* `components/context`: React context components.
* `lib`: Utility modules that have no React/view dependencies.
* `stories`: A storybook implementation of the full app.
* `webui`: The implementations of the application APIs used by the browser.
