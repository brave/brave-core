// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render, screen } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import WorkspaceFileViewer from './workspace_file_viewer'

const kWorkspaceUuid = '0b5c3e2a-8f4d-4c1e-9a7b-2d6f8e1c3a5b'

const workspaceContent: Mojom.AssociatedContent = {
  uuid: 'workspace-1',
  contentType: Mojom.ContentType.Workspace,
  contentId: 1,
  title: 'my-project',
  url: { url: `workspace://${kWorkspaceUuid}` },
  contentUsedPercentage: 100,
  conversationTurnUuid: undefined,
  toolsAttached: true,
}

test('WorkspaceFileViewer frames the workspace page showing the file', () => {
  render(
    <MockContext overrides={{ associatedContent: [workspaceContent] }}>
      <WorkspaceFileViewer path='src/a b&c.tsx' />
    </MockContext>,
  )

  const iframe = screen.getByTitle('a b&c.tsx')
  expect(iframe.tagName).toBe('IFRAME')
  // The path is escaped so it can't add other parameters to the fragment.
  expect(iframe).toHaveAttribute(
    'src',
    `chrome-untrusted://${kWorkspaceUuid}.leo-workspace/#file=src%2Fa%20b%26c.tsx`,
  )
  // The viewer must never be granted WebMCP tools.
  expect(iframe).not.toHaveAttribute('allow')
  expect(screen.getByText('src/a b&c.tsx')).toBeInTheDocument()
})

test('WorkspaceFileViewer expands the file into the lightbox', () => {
  const showWorkspaceFileLightbox = jest.fn()
  render(
    <MockContext
      parentUIFrame={{ showWorkspaceFileLightbox }}
      overrides={{ associatedContent: [workspaceContent] }}
    >
      <WorkspaceFileViewer path='src/app.tsx' />
    </MockContext>,
  )

  // leo-button is a web component, so click the button in its shadow root.
  act(() => {
    screen
      .getByTitle(S.CHAT_UI_WORKSPACE_FILE_EXPAND_BUTTON_LABEL)
      .shadowRoot?.querySelector('button')
      ?.click()
  })
  expect(showWorkspaceFileLightbox).toHaveBeenCalledWith('src/app.tsx')
})

test('WorkspaceFileViewer renders nothing without a workspace', () => {
  const { container } = render(
    <MockContext overrides={{ associatedContent: [] }}>
      <WorkspaceFileViewer path='src/app.tsx' />
    </MockContext>,
  )
  expect(container).toBeEmptyDOMElement()
})

test('WorkspaceFileViewer renders nothing for a non-workspace URL', () => {
  const { container } = render(
    <MockContext
      overrides={{
        associatedContent: [
          { ...workspaceContent, url: { url: 'https://example.com/' } },
        ],
      }}
    >
      <WorkspaceFileViewer path='src/app.tsx' />
    </MockContext>,
  )
  expect(container).toBeEmptyDOMElement()
})
