// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Bubble and side panel share wallet-panel.top-chrome. The side panel boots
// with ?isSidePanel=true, but BrowserRouter navigations drop the query and
// walletReset reloads the current URL — so the marker would be lost and DApp
// prompts could render in the sidebar. Persist for this WebContents.
export const IS_SIDE_PANEL_SESSION_KEY = 'braveWallet.isSidePanel'

export function resolveIsSidePanel(
  search: string = window.location.search,
  storage: Storage = window.sessionStorage,
): boolean {
  const fromQuery = new URLSearchParams(search).get('isSidePanel') === 'true'

  if (fromQuery) {
    try {
      storage.setItem(IS_SIDE_PANEL_SESSION_KEY, 'true')
    } catch {
      // sessionStorage may be unavailable; query is enough for this load.
    }
    return true
  }

  try {
    return storage.getItem(IS_SIDE_PANEL_SESSION_KEY) === 'true'
  } catch {
    return false
  }
}
