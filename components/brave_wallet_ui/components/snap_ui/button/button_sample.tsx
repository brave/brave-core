// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Box, Button, Heading, Text } from '@metamask/snaps-sdk/jsx'

/**
 * Sample UI for `Button`: variants, sizes, `type`, `disabled`, and `loading`.
 *
 * @see https://docs.metamask.io/snaps/reference/snaps-api/snap_createinterface/
 */
export const buttonSampleUi = Box({
  direction: 'vertical',
  children: [
    Heading({ children: 'Button' }),
    Text({
      color: 'muted',
      size: 'sm',
      children:
        'Primary uses Leo filled; destructive uses outline. Sizes map sm → small, md → medium.',
    }),

    Heading({ size: 'sm', children: 'Variant + size (md)' }),
    Box({
      direction: 'horizontal',
      alignment: 'start',
      children: [
        Button({
          name: 'confirm-md',
          children: 'Continue',
          size: 'md',
          variant: 'primary',
        }),
        Button({
          name: 'reject-md',
          children: 'Reject',
          size: 'md',
          variant: 'destructive',
        }),
      ],
    }),

    Heading({ size: 'sm', children: 'Variant + size (sm)' }),
    Box({
      direction: 'horizontal',
      alignment: 'start',
      children: [
        Button({
          name: 'confirm-sm',
          children: 'Continue',
          size: 'sm',
          variant: 'primary',
        }),
        Button({
          name: 'reject-sm',
          children: 'Reject',
          size: 'sm',
          variant: 'destructive',
        }),
      ],
    }),

    Heading({ size: 'sm', children: 'type: button vs submit' }),
    Text({
      size: 'sm',
      children:
        'Wallet receives snapButtonType in onSnapButton for wiring forms later.',
    }),
    Box({
      direction: 'horizontal',
      alignment: 'start',
      children: [
        Button({
          name: 'draft',
          type: 'button',
          children: 'Save draft',
        }),
        Button({
          name: 'send',
          type: 'submit',
          children: 'Submit',
          variant: 'primary',
        }),
      ],
    }),

    Heading({ size: 'sm', children: 'disabled' }),
    Box({
      direction: 'horizontal',
      alignment: 'start',
      children: [
        Button({
          name: 'blocked',
          children: 'Unavailable',
          disabled: true,
          variant: 'primary',
        }),
        Button({
          name: 'blocked-outline',
          children: 'N/A',
          disabled: true,
          variant: 'destructive',
        }),
      ],
    }),

    Heading({ size: 'sm', children: 'loading' }),
    Box({
      direction: 'horizontal',
      alignment: 'start',
      children: [
        Button({
          name: 'working',
          children: 'Working…',
          loading: true,
          variant: 'primary',
        }),
      ],
    }),
  ],
})
