// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Registers a mock BraveNewsController on `window` (and enables the feed), so
// the story renders without connecting to a real Mojo backend. Must come first.
import './storybook/mockBraveNewsController'

import * as React from 'react'
import { BraveNewsContextProvider } from './shared/Context'
import { Sidebar } from './sidebar'

export default {
  title: 'Brave News/Sidebar',
}

export const Default = () => (
  <div style={{ background: '#000' }}>
    <BraveNewsContextProvider>
      <Sidebar />
    </BraveNewsContextProvider>
  </div>
)
