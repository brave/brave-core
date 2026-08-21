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
        conversationHandler={{
          processPermissionChallenge: mockProcessPermissionChallenge,
        }}
      >
        <ToolEvent
          toolUseEvent={{
            toolName: Mojom.NAVIGATE_TOOL_NAME,
            id: '123',
            argumentsJson:
              '{"website_url": "https://www.example.com/path/to/page"}',
            output: undefined,
            permissionChallenge: {
              assessment: 'This is an assessment',
              plan: 'This is a plan',
              description: undefined,
            },
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )
    expect(
      screen.getByText(S.CHAT_UI_PERMISSION_CHALLENGE_HEADER),
    ).toBeInTheDocument()

    expect(screen.getByText('This is an assessment')).toBeInTheDocument()
    expect(screen.getByText('This is a plan')).toBeInTheDocument()

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

  it('should show permission challenge with no content', () => {
    const mockProcessPermissionChallenge = jest.fn()
    render(
      <MockContext
        conversationHandler={{
          processPermissionChallenge: mockProcessPermissionChallenge,
        }}
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
              description: undefined,
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

  it('should show human-readable markdown description when provided', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            // Website-provided (WebMCP) tools have a mangled, model-facing
            // name which should not be shown in the permission prompt.
            toolName: 'web_example_com_get_stock_price',
            id: '123',
            argumentsJson: '{}',
            output: undefined,
            permissionChallenge: {
              assessment: undefined,
              plan: undefined,
              description:
                'Brave AI would like to execute **get_stock_price** '
                + 'on **https://example.com**',
            },
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )
    // The markdown description is rendered with the markdown renderer, with
    // the tool name and origin bolded.
    expect(screen.getByText('get_stock_price')).toBeInTheDocument()
    expect(screen.getByText('get_stock_price').tagName).toBe('STRONG')
    expect(screen.getByText('https://example.com')).toBeInTheDocument()
    expect(
      screen.queryByText(S.CHAT_UI_PERMISSION_CHALLENGE_SUMMARY),
    ).not.toBeInTheDocument()
  })

  it('should not allow permission challenge interaction in a non-active event', () => {
    const mockProcessPermissionChallenge = jest.fn()
    render(
      <MockContext
        conversationHandler={{
          processPermissionChallenge: mockProcessPermissionChallenge,
        }}
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
              description: undefined,
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

  describe('permission challenge tool arguments', () => {
    function renderPermissionChallenge(argumentsJson: string) {
      return render(
        <MockContext>
          <ToolEvent
            toolUseEvent={{
              toolName: Mojom.NAVIGATE_TOOL_NAME,
              id: '123',
              argumentsJson,
              output: undefined,
              permissionChallenge: {
                assessment: undefined,
                plan: undefined,
                description: undefined,
              },
            }}
            isEntryActive={true}
          />
        </MockContext>,
      )
    }

    it('should hide the tool arguments until requested', () => {
      renderPermissionChallenge('{"website_url": "https://www.example.com"}')

      expect(
        screen.getByText(S.CHAT_UI_PERMISSION_CHALLENGE_SHOW_ARGUMENTS_BUTTON),
      ).toBeInTheDocument()
      expect(screen.queryByTestId('tool-arguments')).not.toBeInTheDocument()
    })

    it('should show the tool arguments on click', () => {
      renderPermissionChallenge('{"website_url": "https://www.example.com"}')

      fireEvent.click(screen.getByTestId('tool-arguments-toggle'))

      // The code block splits the text across syntax-highlighted elements, so
      // assert on the rendered text content rather than a single text node.
      const args = screen.getByTestId('tool-arguments')
      expect(args).toHaveTextContent('website_url')
      expect(args).toHaveTextContent('https://www.example.com')
      expect(
        screen.getByText(S.CHAT_UI_PERMISSION_CHALLENGE_HIDE_ARGUMENTS_BUTTON),
      ).toBeInTheDocument()
    })

    it('should hide the tool arguments again on click', () => {
      renderPermissionChallenge('{"website_url": "https://www.example.com"}')

      const toggle = screen.getByTestId('tool-arguments-toggle')
      fireEvent.click(toggle)
      expect(screen.getByTestId('tool-arguments')).toBeInTheDocument()

      fireEvent.click(toggle)
      expect(screen.queryByTestId('tool-arguments')).not.toBeInTheDocument()
    })

    it('should show all arguments, including nested ones', () => {
      renderPermissionChallenge(
        '{"action": "group", "options": {"collapse": true}}',
      )

      fireEvent.click(screen.getByTestId('tool-arguments-toggle'))

      const args = screen.getByTestId('tool-arguments')
      expect(args).toHaveTextContent('action')
      expect(args).toHaveTextContent('group')
      expect(args).toHaveTextContent('options')
      expect(args).toHaveTextContent('collapse')
    })

    it('should show unparseable arguments verbatim', () => {
      renderPermissionChallenge('2 invalid 2 json')

      fireEvent.click(screen.getByTestId('tool-arguments-toggle'))

      expect(screen.getByTestId('tool-arguments')).toHaveTextContent(
        '2 invalid 2 json',
      )
    })

    it('should not offer to show arguments when there are none', () => {
      renderPermissionChallenge('')

      expect(
        screen.queryByTestId('tool-arguments-toggle'),
      ).not.toBeInTheDocument()
    })

    it('should not offer to show arguments for an empty argument object', () => {
      renderPermissionChallenge('{}')

      expect(
        screen.queryByTestId('tool-arguments-toggle'),
      ).not.toBeInTheDocument()
    })
  })
})
