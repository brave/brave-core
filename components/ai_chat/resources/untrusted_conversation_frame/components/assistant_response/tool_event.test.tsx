// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import ToolEvent from './tool_event'

describe('ToolEvent', () => {
  it('should not render tool label for inactive tool events', () => {
    const { container } = render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson: '',
            output: undefined,
          }}
          isEntryActive={false}
        />
      </MockContext>,
    )

    expect(container.innerHTML).toBe('')
  })

  it('should render tool label for active tool events', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson: '',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )
    expect(
      screen.getByText(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE),
    ).toBeInTheDocument()
  })

  it('should handle bad json for active tool events', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson: '2 invalid 2 json',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )
    expect(
      screen.getByText(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE),
    ).toBeInTheDocument()
  })

  it('should handle navigate website url', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson: '{"website_url": "https://www.example.com"}',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )
    expect(
      screen.getByText(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE_WITH_INPUT),
    ).toBeInTheDocument()
  })
})
