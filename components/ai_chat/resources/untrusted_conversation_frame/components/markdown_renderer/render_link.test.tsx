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

  // The citation renders as an anchor (not a button) carrying the destination
  // href, so hovering it discloses the destination via the browser status
  // bubble, like a normal tab.
  const citation = document.querySelector<HTMLAnchorElement>('.citation')
  expect(citation).toBeInTheDocument()
  expect(citation).toBeVisible()
  expect(citation).toHaveTextContent('1')
  expect(citation?.tagName).toBe('A')
  expect(citation?.className).toBe('citation')
  expect(citation?.getAttribute('href')).toBe('https://brave.com')
  expect(citation?.getAttribute('target')).toBe('_blank')
  expect(citation?.getAttribute('rel')).toBe('noopener noreferrer')
})

// A citation source given without a trailing slash still matches the
// canonicalized href, so an equivalent URL renders as a chip.
test('RenderLink treats canonically-equal citation URLs as a match.', () => {
  render(
    <MockContext>
      <RenderLink
        a={{ href: 'https://brave.com/', children: '1' }}
        allowedLinks={['https://brave.com']}
      />
    </MockContext>,
  )
  const citation = document.querySelector<HTMLAnchorElement>('.citation')
  expect(citation).toBeInTheDocument()
  expect(citation?.tagName).toBe('A')
})

// Look-alike URLs must NOT be treated as citations. Citation matching compares
// canonical URL identity, not a string prefix, so none of these masquerade as
// the allowed `https://brave.com` citation source. Each renders as a plain
// anchor (whose destination is still disclosed on hover) rather than a trusted
// citation chip.
test.each([
  { name: 'host suffix', href: 'https://brave.com.evil.example' },
  { name: 'userinfo prefix', href: 'https://brave.com@evil.example' },
  { name: 'path suffix', href: 'https://brave.com.evil.example/brave.com' },
  { name: 'subdomain look-alike', href: 'https://brave.com-evil.example' },
])(
  'RenderLink does not treat $name as a citation of https://brave.com',
  ({ href }) => {
    render(
      <MockContext>
        <RenderLink
          a={{ href, children: '1' }}
          allowedLinks={['https://brave.com']}
        />
      </MockContext>,
    )
    // Never a citation chip...
    expect(document.querySelector('.citation')).not.toBeInTheDocument()
    // ...but still a plain anchor carrying the real (non-trusted) destination.
    const link = screen.getByText('1')
    expect(link.tagName).toBe('A')
    expect(link.className).toBe('conversationLink')
    expect(link.getAttribute('href')).toBe(href)
  },
)

// A citation URL with a fragment is a distinct resource from the allowed
// source and must not render as a citation chip.
test('RenderLink does not treat a fragment variant as a citation.', () => {
  render(
    <MockContext>
      <RenderLink
        a={{ href: 'https://brave.com/#evil', children: '1' }}
        allowedLinks={['https://brave.com/']}
      />
    </MockContext>,
  )
  expect(document.querySelector('.citation')).not.toBeInTheDocument()
  expect(screen.getByText('1').tagName).toBe('A')
})

// A deceptive label ("trusted text" pointing at an attacker URL) is not numeric
// text, so it can never become a citation chip. It renders as a plain anchor
// whose real destination is disclosed on hover.
test('RenderLink renders a deceptive-label link as a plain anchor.', () => {
  render(
    <MockContext>
      <RenderLink
        a={{ href: 'https://attacker.example', children: 'brave.com' }}
        allowedLinks={['https://brave.com']}
      />
    </MockContext>,
  )
  expect(document.querySelector('.citation')).not.toBeInTheDocument()
  const link = screen.getByText('brave.com')
  expect(link.tagName).toBe('A')
  expect(link.className).toBe('conversationLink')
  expect(link.getAttribute('href')).toBe('https://attacker.example')
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
