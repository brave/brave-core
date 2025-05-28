// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import WebSourcesEvent from './web_sources_event'

test('WebSourcesEvent shows chrome-untrusted:// favicon URL as-is', () => {
  render(
    <WebSourcesEvent
      sources={[
        {
          faviconUrl: {
            url: 'chrome-untrusted://resources/brave-icons/globe.svg',
          },
          url: { url: 'https://example.com' },
          title: 'Example Site',
        },
      ]}
    />,
  )

  const img = screen.getByRole('img')
  expect(img).toHaveAttribute(
    'src',
    'chrome-untrusted://resources/brave-icons/globe.svg',
  )
})

test('WebSourcesEvent sanitizes non-chrome-untrusted favicon URL', () => {
  const faviconUrl = 'https://imgs.search.brave.com/favicon.ico'

  render(
    <WebSourcesEvent
      sources={[
        {
          faviconUrl: { url: faviconUrl },
          url: { url: 'https://example.com' },
          title: 'Example Site',
        },
      ]}
    />,
  )

  const img = screen.getByRole('img')
  expect(img).toHaveAttribute(
    'src',
    '//image?url=https%3A%2F%2Fimgs.search.brave.com%2Ffavicon.ico',
  )
})
