// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { render, screen, fireEvent } from '@testing-library/react'
import { createTextContentBlock } from '../../../common/content_block'
import * as Mojom from '../../../common/mojom'
import * as React from 'react'
import MockContext from '../../mock_untrusted_conversation_context'
import ToolEvent from './tool_event'

const validArgumentsJsonThreeChoices =
  '{"choices": ["first", "second", "third"]}'

describe('ToolEventContentUserChoice', () => {
  it('should render clickable choices', () => {
    const mockRespondToToolUseRequest = jest.fn()
    render(
      <MockContext
        conversationHandler={
          {
            respondToToolUseRequest: mockRespondToToolUseRequest,
          } as unknown as Mojom.UntrustedConversationHandlerRemote
        }
      >
        <ToolEvent
          toolUseEvent={{
            toolName: 'user_choice_tool',
            id: '123',
            argumentsJson: validArgumentsJsonThreeChoices,
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(screen.getByTestId('tool-choice-text-0').textContent).toBe('first')
    expect(screen.getByTestId('tool-choice-text-1').textContent).toBe('second')
    expect(screen.getByTestId('tool-choice-text-2').textContent).toBe('third')

    fireEvent.click(screen.getByTestId('tool-choice-text-0'))
    expect(mockRespondToToolUseRequest).toHaveBeenCalledWith('123', [
      {
        textContentBlock: {
          text: 'first',
        },
      },
    ])
  })

  it('should render disabled choices when not active', () => {
    const mockRespondToToolUseRequest = jest.fn()
    render(
      <MockContext
        conversationHandler={
          {
            respondToToolUseRequest: mockRespondToToolUseRequest,
          } as unknown as Mojom.UntrustedConversationHandlerRemote
        }
      >
        <ToolEvent
          toolUseEvent={{
            toolName: 'user_choice_tool',
            id: '123',
            argumentsJson: validArgumentsJsonThreeChoices,
            output: undefined,
          }}
          isEntryActive={false}
        />
      </MockContext>,
    )

    expect(screen.getByTestId('tool-choice-text-0').textContent).toBe('first')
    expect(screen.getByTestId('tool-choice-text-1').textContent).toBe('second')
    expect(screen.getByTestId('tool-choice-text-2').textContent).toBe('third')

    fireEvent.click(screen.getByTestId('tool-choice-text-0'))
    expect(mockRespondToToolUseRequest).not.toHaveBeenCalled()
  })

  it('should render completed state for user choice tool events', () => {
    const mockRespondToToolUseRequest = jest.fn()
    render(
      <MockContext
        conversationHandler={
          {
            respondToToolUseRequest: mockRespondToToolUseRequest,
          } as unknown as Mojom.UntrustedConversationHandlerRemote
        }
      >
        <ToolEvent
          toolUseEvent={{
            toolName: 'user_choice_tool',
            id: '123',
            argumentsJson: '{"choices": ["first", "second", "third"]}',
            output: [createTextContentBlock('first')],
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // should only display result
    expect(screen.getByTestId('tool-choice-text-0').textContent).toBe('first')
    expect(screen.queryByTestId('tool-choice-text-1')).not.toBeInTheDocument()
    expect(screen.queryByTestId('tool-choice-text-2')).not.toBeInTheDocument()

    // shouldn't be able to click when completed
    fireEvent.click(screen.getByTestId('tool-choice-text-0'))
    expect(mockRespondToToolUseRequest).not.toHaveBeenCalled()
  })

  test('should handle bad json input', () => {
    const { container } = render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'user_choice_tool',
            id: '123',
            argumentsJson: '{"choices": }{["first", "second", "third"]}',
            output: undefined,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    expect(container.innerHTML).toBe('')
  })
})
