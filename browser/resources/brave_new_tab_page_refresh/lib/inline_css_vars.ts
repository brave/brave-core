/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

interface CSSVars {
  [key: `--${string}`]: string | number
}

// Allows a collection of CSS custom variables to be used in the "style" prop of
// React components.
export function inlineCSSVars(vars: CSSVars) {
  return vars as React.CSSProperties
}
