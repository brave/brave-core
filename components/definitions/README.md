## Typescript Definition Files

This directory is where we locate Typescript type definitions of Javascript APIs and objects. Only .d.ts files should be present - that is, files that do not result in any extra Javascript.

### What needs a global definition

- Globally-available APIs that are not common outside of Brave - generally at `chrome.*` or `window.*`.
- JS / NPM modules that do not already have Typescript types built-in.

### What does _not_ need a global definition

Types that are local to your specific page or app. Instead, define the type within your component, export it and import it where you need to reference it.
