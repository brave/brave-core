// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { fireEvent, render, screen } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import TabSourcesEvent from './tab_sources_event'

const makeArtifact = (contentJson: string): Mojom.ToolArtifact => ({
  id: null,
  type: Mojom.TAB_SOURCES_ARTIFACT_TYPE,
  contentJson,
})

const twoTabs = makeArtifact(
  JSON.stringify({
    sources: [
      { tab_id: 22, title: 'Tab Two', url: 'https://two.example/docs' },
      { tab_id: 11, title: 'Tab One', url: 'https://one.example/' },
    ],
  }),
)

test('TabSourcesEvent renders a card per tab, in order', () => {
  render(
    <MockContext>
      <TabSourcesEvent artifacts={[twoTabs]} />
    </MockContext>,
  )

  expect(screen.getByTestId('tab-sources-event')).toBeInTheDocument()
  const buttons = screen.getAllByRole('button')
  expect(buttons).toHaveLength(2)
  expect(buttons[0]).toHaveTextContent('Tab Two')
  expect(buttons[1]).toHaveTextContent('Tab One')
})

test('TabSourcesEvent shows the host beneath the title', () => {
  render(
    <MockContext>
      <TabSourcesEvent artifacts={[twoTabs]} />
    </MockContext>,
  )

  expect(screen.getByText('two.example')).toBeInTheDocument()
  expect(screen.getByText('one.example')).toBeInTheDocument()
})

test('TabSourcesEvent falls back to the host when the title is empty', () => {
  render(
    <MockContext>
      <TabSourcesEvent
        artifacts={[
          makeArtifact(
            JSON.stringify({
              sources: [{ tab_id: 7, title: '', url: 'https://only.example/' }],
            }),
          ),
        ]}
      />
    </MockContext>,
  )

  expect(screen.getAllByText('only.example')).toHaveLength(2)
})

test('TabSourcesEvent switches to the clicked tab', () => {
  const switchToTab = jest.fn()

  render(
    <MockContext conversationHandler={{ switchToTab }}>
      <TabSourcesEvent artifacts={[twoTabs]} />
    </MockContext>,
  )

  fireEvent.click(screen.getAllByRole('button')[0])
  expect(switchToTab).toHaveBeenCalledWith(22)
})

test('TabSourcesEvent renders nothing without artifacts', () => {
  const { container } = render(
    <MockContext>
      <TabSourcesEvent artifacts={null} />
    </MockContext>,
  )

  expect(container).toBeEmptyDOMElement()
})

test('TabSourcesEvent renders nothing when no tabs matched', () => {
  const { container } = render(
    <MockContext>
      <TabSourcesEvent
        artifacts={[makeArtifact(JSON.stringify({ sources: [] }))]}
      />
    </MockContext>,
  )

  expect(container).toBeEmptyDOMElement()
})

test('TabSourcesEvent ignores artifacts of other types', () => {
  const { container } = render(
    <MockContext>
      <TabSourcesEvent
        artifacts={[
          {
            id: null,
            type: 'line_chart',
            contentJson: JSON.stringify({ sources: [{ tab_id: 1 }] }),
          },
        ]}
      />
    </MockContext>,
  )

  expect(container).toBeEmptyDOMElement()
})

test.each([
  ['malformed JSON', 'not json'],
  ['a null payload', 'null'],
  ['a payload without sources', '{}'],
  ['a non-array sources value', '{"sources":"nope"}'],
])('TabSourcesEvent ignores %s', (_name, contentJson) => {
  const { container } = render(
    <MockContext>
      <TabSourcesEvent artifacts={[makeArtifact(contentJson)]} />
    </MockContext>,
  )

  expect(container).toBeEmptyDOMElement()
})
