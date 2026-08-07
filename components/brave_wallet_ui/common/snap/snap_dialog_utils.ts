// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Origin attributed to dialogs a snap raises from its own home page (no dApp).
// Must match between the bridge (producer) and the panel (query key).
export const syntheticSnapOrigin = (snapId: string): string => {
  const host = snapId.replace(/[^a-zA-Z0-9._-]/g, '_')
  return `brave://${host}.wallet`
}
