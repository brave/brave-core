// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Append Brave's Tabs and Sidebar sections as additional views in the same
// <cr-view-manager>, shown alongside the Appearance view (see
// currentRouteChanged in the companion appearance_page_index.ts override).
mangle((root) => {
  const viewManager = root.querySelector('cr-view-manager')
  if (!viewManager) {
    throw new Error(
      `[Settings] Appearance page index: couldn't find cr-view-manager`)
  }
  viewManager.insertAdjacentHTML(
    'beforeend',
    `<settings-brave-appearance-tabs id="tabs" slot="view">
     </settings-brave-appearance-tabs>
     <settings-brave-appearance-sidebar id="sidebar" slot="view">
     </settings-brave-appearance-sidebar>`)
})
