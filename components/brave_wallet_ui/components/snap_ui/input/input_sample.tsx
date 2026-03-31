// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Box, Heading, Input, Text } from '@metamask/snaps-sdk/jsx'

/** Sample UI for `Input`: text (default), password, number, disabled. */
export const inputSampleUi = Box({
  direction: 'vertical',
  children: [
    Heading({ children: 'Input' }),
    Text({
      color: 'muted',
      size: 'sm',
      children:
        'Maps to Leo Input. name is stable; value can be updated when the Snap refreshes the interface.',
    }),
    Input({
      name: 'display-name',
      placeholder: 'Display name',
      value: '',
    }),
    Input({
      name: 'password',
      type: 'password',
      placeholder: 'Password',
    }),
    Input({
      name: 'amount',
      type: 'number',
      placeholder: '0',
      min: 0,
      max: 1000,
      step: 1,
      value: '42',
    }),
    Input({
      name: 'readonly-ish',
      type: 'text',
      placeholder: 'Disabled field',
      value: 'Not editable',
      disabled: true,
    }),
  ],
})
