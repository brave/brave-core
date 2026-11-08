/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from '@testing-library/react'
import { EngineIcon } from './engine_icon'

jest.mock(
  '@brave/leo/react/icon',
  () => ({
    __esModule: true,
    default: ({ name }: { name: string }) =>
      require('react').createElement('span', { 'data-icon': name }),
  }),
  { virtual: true },
)

it.each(['', 'https://untrusted.example/icon.png'])(
  'does not load a custom engine favicon (%s)',
  (faviconUrl) => {
    const { container } = render(
      React.createElement(EngineIcon, {
        engine: {
          prepopulateId: 0n,
          name: 'Custom',
          keyword: 'custom',
          host: 'www.google.com',
          faviconUrl,
        },
      }),
    )
    expect(container.querySelector('[data-icon="search"]')).not.toBeNull()
    expect(container.querySelector('img')).toBeNull()
    expect(container.innerHTML).not.toContain('untrusted.example')
  },
)
