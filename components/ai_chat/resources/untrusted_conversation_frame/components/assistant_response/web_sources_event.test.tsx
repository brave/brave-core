// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render, screen } from '@testing-library/react'
import WebSourcesEvent from './web_sources_event'

test('WebSourcesEvent shows chrome-untrusted:// favicon URL as-is', () => {
  render(
    <WebSourcesEvent
      sources={[
        {
          faviconUrl: { url:
            'chrome-untrusted://resources/brave-icons/globe.svg' },
          url: { url: 'https://example.com' },
          title: 'Example Site'
        }
      ]}
    />
  )

  const img = screen.getByRole('img')
  expect(img).toHaveAttribute('src',
      'chrome-untrusted://resources/brave-icons/globe.svg')
})

test('WebSourcesEvent sanitizes non-chrome-untrusted favicon URL', () => {
  const faviconUrl = 'https://imgs.search.brave.com/favicon.ico'

  render(
    <WebSourcesEvent
      sources={[
        {
          faviconUrl: { url: faviconUrl },
          url: { url: 'https://example.com' },
          title: 'Example Site'
        }
      ]}
    />
  )

  const img = screen.getByRole('img')
  expect(img).toHaveAttribute('src',
      '//image?url=https%3A%2F%2Fimgs.search.brave.com%2Ffavicon.ico')
})

test('Unhidden WebSourcesEvent has index citation number', () => {
  render(
    <WebSourcesEvent
      sources={[
        {
          faviconUrl: { url:
            'chrome-untrusted://resources/brave-icons/globe.svg' },
          url: { url: 'https://example.com' },
          title: 'Example Site'
        },
        {
          faviconUrl: { url:
            'chrome-untrusted://resources/brave-icons/globe.svg' },
          url: { url: 'https://example2.com' },
          title: 'Example Site 2'
        }
      ]}
    />
  )
  expect(screen.getByText('1 - example.com')).toBeInTheDocument()
  expect(screen.getByText('2 - example2.com')).toBeInTheDocument()
})

test('Hidden WebSourcesEvent citation numbers continue after expand', () => {
  render(
    <WebSourcesEvent
      sources={[1, 2, 3, 4, 5, 6].map((source) => ({
        faviconUrl: { url:
          'chrome-untrusted://resources/brave-icons/globe.svg' },
        url: { url: `https://example${source}.com` },
        title: `Example Site ${source}`
      }))}
    />
  )

  // Test first 4 unhidden sources
  expect(screen.getByText('1 - example1.com')).toBeInTheDocument()
  expect(screen.getByText('2 - example2.com')).toBeInTheDocument()
  expect(screen.getByText('3 - example3.com')).toBeInTheDocument()
  expect(screen.getByText('4 - example4.com')).toBeInTheDocument()

  // Test expand button
  const expandButton =
    document.querySelector<HTMLButtonElement>('button[name="expand"]')
  expect(expandButton).toBeInTheDocument()
  expect(expandButton).toBeVisible()
  act(() => expandButton?.click())

  // Test hidden sources
  expect(screen.getByText('5 - example5.com')).toBeInTheDocument()
  expect(screen.getByText('6 - example6.com')).toBeInTheDocument()

  // Test expand button is not visible
  expect(expandButton).not.toBeInTheDocument()
  expect(expandButton).not.toBeVisible()
})
