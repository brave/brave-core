// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Box, Heading, Text } from '@metamask/snaps-sdk/jsx'

/** Sample UI focused on `Heading` sizes (`sm`, `md`, `lg`). */
export const headingSampleUi = Box({
  direction: 'vertical',
  children: [
    Heading({ children: 'Heading sizes' }),
    Text({ children: 'MetaMask Snaps supports sm / md / lg.' }),
    Heading({ size: 'sm', children: 'Heading size sm' }),
    Heading({ size: 'md', children: 'Heading size md' }),
    Heading({ size: 'lg', children: 'Heading size lg' }),
  ],
})
