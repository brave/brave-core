// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import { RenderLink } from '.'

test('Test RenderLink component with allowed links.', async () => {
  render(
    <RenderLink
      a={{ href: 'https://example.com', children: 'Test Link' }}
      allowedLinks={['https://example.com']}
    />,
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('A')
  expect(screen.getByText('Test Link').className).toBe('conversationLink')
})

test('Test RenderLink component with disallowed links.', async () => {
  render(
    <RenderLink
      a={{ href: 'https://example.com', children: 'Test Link' }}
      allowedLinks={['https://brave.com']}
    />,
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('SPAN')
  expect(screen.getByText('Test Link').className).toBe('')
})

test('Test RenderLink component with citations.', async () => {
  render(
    <RenderLink
      a={{ href: 'https://brave.com', children: '1' }}
      allowedLinks={['https://brave.com']}
    />,
  )
  expect(screen.getByText('1')).toBeInTheDocument()
  expect(screen.getByText('1').tagName).toBe('A')
  expect(screen.getByText('1').className).toBe('conversationLink citation')
})

test('Test RenderLink component with disableLinkRestrictions.', async () => {
  render(
    <RenderLink
      a={{ href: 'https://example.com', children: 'Test Link' }}
      allowedLinks={[]}
      disableLinkRestrictions={true}
    />,
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('A')
  expect(screen.getByText('Test Link').className).toBe('conversationLink')
})

// HTTP links should never be allowed
test('Test RenderLink component with http links.', async () => {
  render(
    <RenderLink
      a={{ href: 'http://example.com', children: 'Test Link' }}
      allowedLinks={['http://example.com']}
      disableLinkRestrictions={true}
    />,
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('SPAN')
  expect(screen.getByText('Test Link').className).toBe('')
})
