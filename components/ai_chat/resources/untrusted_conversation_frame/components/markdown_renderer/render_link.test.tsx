// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import MockContext from '../../mock_untrusted_conversation_context'
import { RenderLink } from '.'

// All HTTPS links are allowed.
test('RenderLink component with https links.', async () => {
  render(
    <MockContext>
      <RenderLink a={{ href: 'https://example.com', children: 'Test Link' }} />
    </MockContext>,
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('A')
  expect(screen.getByText('Test Link').className).toBe('conversationLink')
})

// Relative links are completely hidden.
test('RenderLink component with relative links.', async () => {
  render(
    <MockContext>
      <RenderLink a={{ href: '/some/path', children: 'Test Link' }} />
    </MockContext>,
  )
  expect(screen.queryByText('Test Link')).not.toBeInTheDocument()
})

// A numeric-text link that points at a citation source renders as a chip.
test('RenderLink component with citations.', async () => {
  render(
    <MockContext>
      <RenderLink
        a={{ href: 'https://brave.com', children: '1' }}
        allowedLinks={['https://brave.com']}
      />
    </MockContext>,
  )

  // Make sure the label is visible
  const label = document.querySelector<HTMLDivElement>('leo-label')
  expect(label).toBeInTheDocument()
  expect(label).toBeVisible()
  expect(label).toHaveTextContent('1')

  // Make sure the citation is visible
  const citation = document.querySelector<HTMLButtonElement>('.citation')
  expect(citation).toBeInTheDocument()
  expect(citation).toBeVisible()
  expect(citation).toHaveTextContent('1')
  expect(citation?.tagName).toBe('BUTTON')
  expect(citation?.className).toBe('citation')
})

// A numeric-text link that is not a citation source renders as a plain anchor,
// not a citation chip.
test('RenderLink component with numeric link outside allowedLinks.', async () => {
  render(
    <MockContext>
      <RenderLink
        a={{ href: 'https://example.com', children: '1' }}
        allowedLinks={['https://brave.com']}
      />
    </MockContext>,
  )
  expect(document.querySelector('.citation')).not.toBeInTheDocument()
  const link = screen.getByText('1')
  expect(link.tagName).toBe('A')
  expect(link.className).toBe('conversationLink')
})

// HTTP links should never be allowed
test('RenderLink component with http links.', async () => {
  render(
    <MockContext>
      <RenderLink a={{ href: 'http://example.com', children: 'Test Link' }} />
    </MockContext>,
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('SPAN')
  expect(screen.getByText('Test Link').className).toBe('')
})

// Only HTTPS is allowed. Every other scheme - including dangerous ones like
// javascript: and privileged ones like chrome:/chrome-untrusted: - renders the
// link text as a plain <span>, so it cannot be navigated to or executed.
test.each([
  // Intentionally testing that a javascript: URL is rendered inert, not run.
  // eslint-disable-next-line no-script-url
  { scheme: 'javascript', href: 'javascript:alert(1)' },
  { scheme: 'chrome', href: 'chrome://settings' },
  { scheme: 'chrome-untrusted', href: 'chrome-untrusted://resources/foo' },
])('RenderLink renders $scheme: links as plain text', async ({ href }) => {
  render(
    <MockContext>
      <RenderLink a={{ href, children: 'Test Link' }} />
    </MockContext>,
  )
  const link = screen.getByText('Test Link')
  expect(link).toBeInTheDocument()
  expect(link.tagName).toBe('SPAN')
  expect(link.className).toBe('')
})
