// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { fireEvent, render, screen, waitFor } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import InlineTabSearch from './inline_tab_search'

const twoTabs: Mojom.TabData[] = [
  {
    id: 22,
    contentId: 220,
    title: 'Tab Two',
    url: { url: 'https://two.example/docs' },
  },
  {
    id: 11,
    contentId: 110,
    title: 'Tab One',
    url: { url: 'https://one.example/' },
  },
]

test('InlineTabSearch renders a card per matched tab, in order', async () => {
  render(
    <MockContext uiHandler={{ searchForTabs: async () => ({ tabs: twoTabs }) }}>
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  await waitFor(() =>
    expect(screen.getByTestId('inline-tab-search')).toBeInTheDocument(),
  )
  const buttons = screen.getAllByRole('button')
  expect(buttons).toHaveLength(2)
  expect(buttons[0]).toHaveTextContent('Tab Two')
  expect(buttons[1]).toHaveTextContent('Tab One')
})

test('InlineTabSearch passes the directive query to the browser', async () => {
  const searchForTabs = jest.fn(async () => ({ tabs: twoTabs }))

  render(
    <MockContext uiHandler={{ searchForTabs }}>
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  await waitFor(() => expect(searchForTabs).toHaveBeenCalledWith('react hooks'))
})

test('InlineTabSearch keeps favicon lookups on device', async () => {
  const { container } = render(
    <MockContext uiHandler={{ searchForTabs: async () => ({ tabs: twoTabs }) }}>
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  await waitFor(() => expect(container.querySelectorAll('img')).toHaveLength(2))
  for (const img of container.querySelectorAll('img')) {
    // Falling back to Google's favicon server would send the tab URL off
    // device.
    expect(img).toHaveAttribute(
      'src',
      expect.stringContaining('allowGoogleServerFallback=0'),
    )
  }
})

test('InlineTabSearch switches to the clicked tab', async () => {
  const switchToTab = jest.fn()

  render(
    <MockContext
      uiHandler={{
        searchForTabs: async () => ({ tabs: twoTabs }),
        switchToTab,
      }}
    >
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  await waitFor(() =>
    expect(screen.getByTestId('inline-tab-search')).toBeInTheDocument(),
  )
  fireEvent.click(screen.getAllByRole('button')[0])
  expect(switchToTab).toHaveBeenCalledWith(22)
})

test('InlineTabSearch falls back to the host when a tab has no title', async () => {
  render(
    <MockContext
      uiHandler={{
        searchForTabs: async () => ({
          tabs: [
            {
              id: 7,
              contentId: 70,
              title: '',
              url: { url: 'https://only.example/' },
            },
          ],
        }),
      }}
    >
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  // Title falls back to the host; the subtitle is the scheme-stripped URL.
  await waitFor(() =>
    expect(screen.getByText('only.example')).toBeInTheDocument(),
  )
  expect(screen.getByText('only.example/')).toBeInTheDocument()
})

test('InlineTabSearch reports when no open tab matched', async () => {
  render(
    <MockContext uiHandler={{ searchForTabs: async () => ({ tabs: [] }) }}>
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  await waitFor(() =>
    expect(screen.getByTestId('inline-tab-search-empty')).toBeInTheDocument(),
  )
  expect(screen.queryByTestId('inline-tab-search')).not.toBeInTheDocument()
})

test('InlineTabSearch reports progress while the search runs', () => {
  render(
    <MockContext
      uiHandler={{ searchForTabs: () => new Promise(() => {}) as any }}
    >
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  expect(screen.getByTestId('inline-tab-search-pending')).toBeInTheDocument()
})

test('InlineTabSearch renders nothing where tab search is unavailable', async () => {
  const { container } = render(
    <MockContext uiHandler={{ searchForTabs: async () => ({ tabs: null }) }}>
      <InlineTabSearch query='react hooks' />
    </MockContext>,
  )

  // Null means the platform has no on-device tab search, which shouldn't
  // surface as a "no matches" message mid-answer.
  await waitFor(() => expect(container).toBeEmptyDOMElement())
})
