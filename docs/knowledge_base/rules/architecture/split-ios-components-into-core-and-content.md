---
name: brave/split-ios-components-into-core-and-content
description:
  'A component that must work on both iOS and content-based platforms is split
  into core/ (business logic, no //content) and content/ (content-layer glue).'
rubric: design
severity: should_fix
---

**Rule.** When a component needs to work on iOS as well as on content-based
platforms, lay it out as `core/` (iOS-compatible: `core/browser/`,
`core/common/`, no `//content` dependency) and `content/` (`content/browser/`,
code using `//content` APIs such as `WebContents`). Business logic goes in
`core/`; `content/` holds only the content-layer glue.

**Why.** This is the layered component structure of Chromium's componentization
cookbook: iOS builds pull in only `core/`, so the logic both platforms share has
to live there, free of content types.

## Examples

Bad:

```
components/brave_shields/browser/   # logic and WebContents code mixed
```

Good:

```
components/brave_shields/
├── core/           # iOS-compatible code, no //content deps
│   ├── browser/
│   └── common/
└── content/        # Content-layer integration (WebContents, etc.)
    └── browser/
```
