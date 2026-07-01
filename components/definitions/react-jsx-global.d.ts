// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// React 19's @types/react removed the global `JSX` namespace; the JSX types now
// live under `React.JSX`, and with `jsx: react` TypeScript resolves intrinsic
// elements from `React.JSX.IntrinsicElements`.
//
// Two things still rely on the old global `JSX` namespace:
//   1. A lot of our code (and some third-party .d.ts) references global JSX
//      types directly, e.g. `JSX.Element`.
//   2. Libraries like @brave/leo (and a few of our own files) register custom
//      elements (`leo-menu-item`, `leo-title`, ...) by augmenting the *global*
//      `JSX.IntrinsicElements` interface.
//
// This file restores a global `JSX` namespace (aliasing `React.JSX`) so (1)
// keeps compiling, and bridges the global `JSX.IntrinsicElements` into
// `React.JSX.IntrinsicElements` so the custom-element augmentations in (2) are
// visible to the element resolver.
// https://react.dev/blog/2024/04/25/react-19-upgrade-guide#the-jsx-namespace-in-typescript
import type { JSX as ReactJSX } from 'react'

declare global {
  namespace JSX {
    type ElementType = ReactJSX.ElementType
    interface Element extends ReactJSX.Element {}
    interface ElementClass extends ReactJSX.ElementClass {}
    interface ElementAttributesProperty
      extends ReactJSX.ElementAttributesProperty {}
    interface ElementChildrenAttribute
      extends ReactJSX.ElementChildrenAttribute {}
    type LibraryManagedAttributes<C, P> =
      ReactJSX.LibraryManagedAttributes<C, P>
    interface IntrinsicAttributes extends ReactJSX.IntrinsicAttributes {}
    interface IntrinsicClassAttributes<T>
      extends ReactJSX.IntrinsicClassAttributes<T> {}
    // Extension point for global custom-element augmentations (leo-*, etc.).
    // Intentionally does not extend ReactJSX.IntrinsicElements to avoid a
    // circular reference with the module augmentation below.
    interface IntrinsicElements {}
  }

  // Alias so the `react` module augmentation below can reference the global
  // JSX.IntrinsicElements (the name `JSX` inside `declare module 'react'`
  // would otherwise resolve to React's own JSX namespace).
  type BraveGlobalJSXIntrinsicElements = JSX.IntrinsicElements
}

declare module 'react' {
  namespace JSX {
    interface IntrinsicElements extends BraveGlobalJSXIntrinsicElements {}
  }
}
