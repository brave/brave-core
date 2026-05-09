// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { braveNewsCardClickHandler } from './Card'

jest.mock('../shared/configurationCache', () => ({
  ConfigurationCachingWrapper: {
    getInstance: () => ({
      value: {
        openArticlesInNewTab: false,
      },
    }),
  },
}))

describe('braveNewsCardClickHandler', () => {
  const makeEvent = (overrides?: Partial<React.MouseEvent>) => ({
    ctrlKey: false,
    metaKey: false,
    buttons: 0,
    ...overrides,
  }) as React.MouseEvent

  it('allows https ad targets when https is allowlisted', () => {
    const openSpy = jest.spyOn(window, 'open').mockImplementation(() => null)

    const handler = braveNewsCardClickHandler('https://brave.com', ['https:'])
    handler(makeEvent({ ctrlKey: true }))

    expect(openSpy).toHaveBeenCalledWith(
      'https://brave.com',
      '_blank',
      'noopener noreferrer'
    )
  })

  it.each([
    'http://example.com',
    'chrome://settings',
    'brave://settings',
    'file:///tmp/example.txt',
    'javascript:alert(1)',
  ])('rejects disallowed ad target scheme: %s', (url) => {
    const handler = braveNewsCardClickHandler(url, ['https:'])

    expect(() => handler(makeEvent())).toThrow('Unsupported scheme')
  })
})
