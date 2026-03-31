// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Box, Heading, Text } from '@metamask/snaps-sdk/jsx'

/** Sample UI focused on `Box` direction and alignment. */
export const boxSampleUi = Box({
  direction: 'vertical',
  children: [
    Heading({ children: 'Box layout' }),
    Text({ children: 'Vertical stack (default):' }),
    Box({
      direction: 'vertical',
      alignment: 'start',
      children: [
        Text({ children: 'First row' }),
        Text({ children: 'Second row' }),
      ],
    }),
    Text({ children: 'Horizontal row, centered:' }),
    Box({
      direction: 'horizontal',
      alignment: 'center',
      crossAlignment: 'center',
      children: [Text({ children: 'Left' }), Text({ children: 'Right' })],
    }),
  ],
})
