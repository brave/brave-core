// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Converts `A | B | C` to `A & B & C`
type UnionToIntersection<U> =
  (U extends any ? (x: U) => void : never) extends ((x: infer I) => void) ? I : never

declare global {
    // Global interface containing all available strings. This lets WebUIs
    // extend the available strings via
    // import { MyStrings } from 'gen/components/grit/brave_components_webui_strings'
    // declare global {
    //   interface Strings {
    //     MyStrings: typeof MyStrings
    //   }
    // }
    // and have them available in the global scope as `S.MY_STRINGS_XXX`
    interface Strings {
    }

    // S is the intersection of all the enums exposed on the Strings interface.
    const S: UnionToIntersection<Strings[keyof Strings]>
}

// Empty default export to make this a module
export {}
