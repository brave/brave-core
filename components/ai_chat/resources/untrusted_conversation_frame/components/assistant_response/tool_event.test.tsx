// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { fireEvent, render, screen } from '@testing-library/react'
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

  it('should show expanded content on click', () => {
    const result = render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.ASSISTANT_DETAIL_STORAGE_TOOL_NAME,
            id: '123',
            argumentsJson: '{"information": "This is some information"}',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    const toolLabel = screen.getByText(
      S.CHAT_UI_TOOL_LABEL_ASSISTANT_DETAIL_STORAGE,
    )
    expect(toolLabel).toBeInTheDocument()
    // Click the label and check if the expanded content is shown
    fireEvent.click(toolLabel)
    expect(result.getByText('This is some information')).toBeInTheDocument()
  })

  it('should show permission challenge', () => {
    const mockProcessPermissionChallenge = jest.fn()
    render(
      <MockContext
        conversationHandler={
          {
            processPermissionChallenge: mockProcessPermissionChallenge,
          } as unknown as Mojom.UntrustedConversationHandlerRemote
        }
      >
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson:
              '{"website_url": "https://www.example.com/path/to/page"}',
            output: undefined,
            permissionChallenge: {
              // No content to verify they do not gate the rendering of the
              // permission challenge UI.
              assessment: undefined,
              plan: undefined,
            },
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )
    expect(
      screen.getByText(S.CHAT_UI_PERMISSION_CHALLENGE_HEADER),
    ).toBeInTheDocument()
    const approveButton = screen.getByText(
      S.CHAT_UI_PERMISSION_CHALLENGE_ALLOW_BUTTON,
    )
    fireEvent.click(approveButton)
    expect(mockProcessPermissionChallenge).toHaveBeenCalledWith('123', true)
    const denyButton = screen.getByText(
      S.CHAT_UI_PERMISSION_CHALLENGE_DENY_BUTTON,
    )
    fireEvent.click(denyButton)
    expect(mockProcessPermissionChallenge).toHaveBeenCalledWith('123', false)
  })

  it('should not allow permission challenge interaction in a non-active event', () => {
    const mockProcessPermissionChallenge = jest.fn()
    render(
      <MockContext
        conversationHandler={
          {
            processPermissionChallenge: mockProcessPermissionChallenge,
          } as unknown as Mojom.UntrustedConversationHandlerRemote
        }
      >
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson:
              '{"website_url": "https://www.example.com/path/to/page"}',
            output: undefined,
            permissionChallenge: {
              // No content to verify they do not gate the rendering of the
              // permission challenge UI.
              assessment: undefined,
              plan: undefined,
            },
          }}
          isEntryActive={false}
        />
      </MockContext>,
    )
    expect(
      screen.getByText(S.CHAT_UI_PERMISSION_CHALLENGE_HEADER),
    ).toBeInTheDocument()
    const approveButton = screen.getByText(
      S.CHAT_UI_PERMISSION_CHALLENGE_ALLOW_BUTTON,
    )
    fireEvent.click(approveButton)
    expect(mockProcessPermissionChallenge).not.toHaveBeenCalled()
    const denyButton = screen.getByText(
      S.CHAT_UI_PERMISSION_CHALLENGE_DENY_BUTTON,
    )
    fireEvent.click(denyButton)
    expect(mockProcessPermissionChallenge).not.toHaveBeenCalled()
  })
})
