// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Bold, Box, Heading, Italic, Text } from '@metamask/snaps-sdk/jsx'

/** Sample UI focused on `Text` colors, sizes, weight, and inline formatting. */
export const textSampleUi = Box({
  direction: 'vertical',
  children: [
    Heading({ children: 'Text styles' }),
    Text({
      children: 'Default body text.',
    }),
    Text({
      color: 'muted',
      size: 'sm',
      children: 'Muted, small.',
    }),
    Text({
      color: 'error',
      fontWeight: 'bold',
      children: 'Error emphasis.',
    }),
    Text({
      alignment: 'center',
      children: [
        'Centered with ',
        Bold({ children: 'bold' }),
        ' and ',
        Italic({ children: 'italic' }),
        '.',
      ],
    }),
  ],
})
