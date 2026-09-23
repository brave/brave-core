// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { fireEvent, render, screen } from '@testing-library/react'
import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import {
  createConversationTurnWithDefaults,
  getCompletionEvent,
  getEventTemplate,
  getToolUseEvent,
} from '../../../common/test_data_utils'
import MockContext from '../../mock_untrusted_conversation_context'
import ProgressBubble, {
  ProgressBubbleContextProvider,
  useProgressBubbleContext,
} from './progress_bubble'

// Helpers ---------------------------------------------------------------------

function getBubble() {
  // The bubble's outer element has the `bubble` class (identity-obj-proxy
  // returns CSS module keys verbatim). Any descendant text lives inside it.
  return document.querySelector<HTMLElement>('.bubble')
}

function makeAssistantTurn(events: Mojom.ConversationEntryEvent[]) {
  return createConversationTurnWithDefaults({
    characterType: Mojom.CharacterType.ASSISTANT,
    events,
  })
}

// A task group is any assistant group with >=1 (non-USER_CHOICE,
// non-MEMORY_STORAGE) tool use event.
function makeTaskGroup(): Mojom.ConversationTurn[] {
  return [
    makeAssistantTurn([
      getToolUseEvent({
        toolName: Mojom.NAVIGATE_TOOL_NAME,
        id: '1',
        argumentsJson: '{"website_url":"https://example.com"}',
        output: undefined,
      }),
    ]),
    makeAssistantTurn([getCompletionEvent('Done.')]),
  ]
}

function makeNonTaskGroup(): Mojom.ConversationTurn[] {
  return [makeAssistantTurn([getCompletionEvent('Plain answer.')])]
}

// -----------------------------------------------------------------------------
// In-progress display
// -----------------------------------------------------------------------------

describe('ProgressBubble in-progress display', () => {
  test('shows "Thinking" when generating with no responseGroup yet (just after human submit)', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: true },
        }}
      >
        <ProgressBubble
          responseGroup={undefined}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TOOL_LABEL_THINKING)).toBeInTheDocument()
    expect(getBubble()?.className).toContain('isActive')
    expect(getBubble()?.className).not.toContain('isExpandable')
  })

  test('shows "Thinking" for a non-task (no tool use) last group while generating', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: true },
        }}
      >
        <ProgressBubble
          responseGroup={makeNonTaskGroup()}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TOOL_LABEL_THINKING)).toBeInTheDocument()
  })

  test('shows a tool-specific label when the latest event is a known tool use', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: true },
        }}
      >
        <ProgressBubble
          responseGroup={[
            makeAssistantTurn([
              getCompletionEvent('Working...'),
              getToolUseEvent({
                toolName: Mojom.NAVIGATE_TOOL_NAME,
                id: '1',
                argumentsJson: '{"website_url":"https://brave.com"}',
                output: undefined,
              }),
            ]),
          ]}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(
      screen.getByText(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE_WITH_INPUT),
    ).toBeInTheDocument()
    // Not the fallback
    expect(
      screen.queryByText(S.CHAT_UI_TOOL_LABEL_THINKING),
    ).not.toBeInTheDocument()
  })

  test('falls back to tool name when the latest tool has no label mapping', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: true },
        }}
      >
        <ProgressBubble
          responseGroup={[
            makeAssistantTurn([
              getToolUseEvent({
                toolName: 'unknown_tool',
                id: '1',
                argumentsJson: '{}',
                output: undefined,
              }),
            ]),
          ]}
          isLastGroup={true}
        />
      </MockContext>,
    )

    // getToolLabel falls back to the tool name itself for unknown tools,
    // which is what should be displayed.
    expect(screen.getByText('unknown_tool')).toBeInTheDocument()
  })

  test('ignores filler events (sources, search, content receipt) when picking the latest event', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: true },
        }}
      >
        <ProgressBubble
          responseGroup={[
            makeAssistantTurn([
              getToolUseEvent({
                toolName: Mojom.NAVIGATE_TOOL_NAME,
                id: '1',
                argumentsJson: '{"website_url":"https://brave.com"}',
                output: undefined,
              }),
              // These should not displace the navigate tool as the "latest"
              // representative event.
              {
                ...getEventTemplate(),
                searchQueriesEvent: { searchQueries: ['q'] },
              },
              {
                ...getEventTemplate(),
                sourcesEvent: { sources: [], richResults: [] },
              },
            ]),
          ]}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(
      screen.getByText(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE_WITH_INPUT),
    ).toBeInTheDocument()
  })

  test('shows in-progress state when isToolExecuting (even if not isGenerating)', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            isGenerating: false,
            isToolExecuting: true,
          },
        }}
      >
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(getBubble()?.className).toContain('isActive')
    expect(getBubble()?.className).not.toContain('isInterrupted')
  })
})

// -----------------------------------------------------------------------------
// Hidden states
// -----------------------------------------------------------------------------

describe('ProgressBubble hidden states', () => {
  test('renders nothing for a non-task group when not the latest and not generating', () => {
    render(
      <MockContext>
        <ProgressBubble
          responseGroup={makeNonTaskGroup()}
          isLastGroup={false}
        />
      </MockContext>,
    )

    expect(getBubble()).toBeNull()
  })

  test('renders nothingfor non-last group, even while generating', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: true },
        }}
      >
        <ProgressBubble
          responseGroup={makeNonTaskGroup()}
          isLastGroup={false}
        />
      </MockContext>,
    )

    expect(getBubble()).toBeNull()
  })

  test('renders nothing for a non-task latest group when the conversation is not generating', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: { isGenerating: false },
        }}
      >
        <ProgressBubble
          responseGroup={makeNonTaskGroup()}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(getBubble()).toBeNull()
  })

  test('renders nothing when there is no responseGroup and not generating', () => {
    render(
      <MockContext>
        <ProgressBubble
          responseGroup={undefined}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(getBubble()).toBeNull()
  })

  test('renders nothing for a task-only USER_CHOICE/MEMORY_STORAGE group (not a task)', () => {
    render(
      <MockContext>
        <ProgressBubble
          responseGroup={[
            makeAssistantTurn([
              getToolUseEvent({
                toolName: Mojom.USER_CHOICE_TOOL_NAME,
                id: '1',
                argumentsJson: '{}',
                output: undefined,
              }),
              getCompletionEvent('Pick one'),
            ]),
          ]}
          isLastGroup={false}
        />
      </MockContext>,
    )

    expect(getBubble()).toBeNull()
  })
})

// -----------------------------------------------------------------------------
// Completed state
// -----------------------------------------------------------------------------

describe('ProgressBubble completed state', () => {
  test('shows "Task complete" for a task group that is not the latest', () => {
    render(
      <MockContext>
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={false}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TOOL_LABEL_COMPLETE)).toBeInTheDocument()
    const bubble = getBubble()
    expect(bubble?.className).not.toContain('isActive')
    expect(bubble?.className).not.toContain('isInterrupted')
    expect(bubble?.className).toContain('isExpandable')
  })

  test('shows "Task complete" for a non-active task group in an active conversation', () => {
    // The conversation-wide isGenerating/isToolExecuting flags only describe
    // the active group. A non-active task group should ignore them and stay
    // in the completed state.
    render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            isGenerating: true,
            isToolExecuting: true,
            toolUseTaskState: Mojom.TaskState.kRunning,
          },
        }}
      >
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={false}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TOOL_LABEL_COMPLETE)).toBeInTheDocument()
    const bubble = getBubble()
    expect(bubble?.className).not.toContain('isActive')
    expect(bubble?.className).toContain('isExpandable')
  })

  test('shows "Task complete" for the latest task group when not generating', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            isGenerating: false,
            isToolExecuting: false,
            toolUseTaskState: Mojom.TaskState.kNone,
          },
        }}
      >
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TOOL_LABEL_COMPLETE)).toBeInTheDocument()
    expect(getBubble()?.className).not.toContain('isInterrupted')
  })

  test('is expandable: clicking the bubble toggles the ProgressBubbleContext', () => {
    // scrollIntoView is not implemented in jsdom but the bubble's click
    // handler invokes it on the sentinel element.
    Element.prototype.scrollIntoView = jest.fn()

    function ExpandedFlag() {
      const ctx = useProgressBubbleContext()
      return (
        <span data-testid='is-expanded'>{ctx.isExpanded ? 'yes' : 'no'}</span>
      )
    }

    render(
      <MockContext>
        <ProgressBubbleContextProvider>
          <ProgressBubble
            responseGroup={makeTaskGroup()}
            isLastGroup={false}
          />
          <ExpandedFlag />
        </ProgressBubbleContextProvider>
      </MockContext>,
    )

    expect(screen.getByTestId('is-expanded')).toHaveTextContent('no')

    fireEvent.click(getBubble()!)
    expect(screen.getByTestId('is-expanded')).toHaveTextContent('yes')

    fireEvent.click(getBubble()!)
    expect(screen.getByTestId('is-expanded')).toHaveTextContent('no')
  })
})

// -----------------------------------------------------------------------------
// Interrupted state
// -----------------------------------------------------------------------------

describe('ProgressBubble interrupted state', () => {
  test('shows "Paused" when latest task group and toolUseTaskState is kPaused', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            isGenerating: false,
            isToolExecuting: false,
            toolUseTaskState: Mojom.TaskState.kPaused,
          },
        }}
      >
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(
      screen.getByText(S.CHAT_UI_TASK_STATE_PAUSED_LABEL),
    ).toBeInTheDocument()
    expect(getBubble()?.className).toContain('isInterrupted')
  })

  test('shows "Stopped" when latest task group and toolUseTaskState is kStopped', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            isGenerating: false,
            isToolExecuting: false,
            toolUseTaskState: Mojom.TaskState.kStopped,
          },
        }}
      >
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={true}
        />
      </MockContext>,
    )

    expect(
      screen.getByText(S.CHAT_UI_TASK_STATE_STOPPED_LABEL),
    ).toBeInTheDocument()
    expect(getBubble()?.className).toContain('isInterrupted')
  })

  test('does not show interrupted state on non-latest task groups (state belongs to the active task)', () => {
    render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            isGenerating: false,
            isToolExecuting: false,
            toolUseTaskState: Mojom.TaskState.kPaused,
          },
        }}
      >
        <ProgressBubble
          responseGroup={makeTaskGroup()}
          isLastGroup={false}
        />
      </MockContext>,
    )

    expect(screen.getByText(S.CHAT_UI_TOOL_LABEL_COMPLETE)).toBeInTheDocument()
    expect(
      screen.queryByText(S.CHAT_UI_TASK_STATE_PAUSED_LABEL),
    ).not.toBeInTheDocument()
    expect(getBubble()?.className).not.toContain('isInterrupted')
  })
})
