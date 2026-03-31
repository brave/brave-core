// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Serialized Snaps custom UI node (output of `@metamask/snaps-sdk/jsx`
 * components). Matches MetaMask's snap UI element shape.
 */
export interface SnapUiElement {
  type: string
  props: Record<string, unknown>
  key: string | number | null
}
