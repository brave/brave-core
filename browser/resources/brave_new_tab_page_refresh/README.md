# Brave NTP

## Overview

Brave NTP is a React application organized into the following folders:

* `assets`: Images and other resources (e.g. SVGs) that will be bundled into the
application
* `components`: The React components implementing the application view layer
* `components/common`: Generically useful helper views
* `context`: Providers and associated hooks for any context that will be made
available to the application or subtrees of the application
* `lib`: Small helper and utility modules
* `state`: App-wide state definitions and associated action interfaces. The
modules in this folder should not contain dependencies on React or the view
layer.
* `stories`: Storybook stories used for rapid UI development, along with
supporting action handlers.
