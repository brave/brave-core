// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { fireEvent, render, screen, waitFor } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import ToolPermissionChallenge from './tool_permission_challenge'

const mockProcessPermissionChallenge = jest.fn()

jest.mock('../../untrusted_conversation_context', () => ({
  useUntrustedConversationContext: () => ({
    userActions: {
      processPermissionChallenge: (
        id: string,
        decision: Mojom.PermissionChallengeDecision,
      ) => mockProcessPermissionChallenge(id, decision),
    },
  }),
}))

function toolUseEvent(
  toolName: string,
  implications?: string,
  supportsAllowSession = false,
): Mojom.ToolUseEvent {
  return {
    toolName,
    id: 'tool-use-id',
    argumentsJson: '{}',
    output: undefined,
    isServerResult: false,
    artifacts: undefined,
    permissionChallenge: {
      assessment: undefined,
      plan: undefined,
      title: undefined,
      description: undefined,
      implications,
      supportsAllowSession,
    },
  }
}

function renderChallenge(event: Mojom.ToolUseEvent, isInteractive = true) {
  return render(
    <ToolPermissionChallenge
      isInteractive={isInteractive}
      toolUseEvent={event}
      toolLabel='Managing tabs'
    />,
  )
}

async function decisionButton(label: string): Promise<HTMLButtonElement> {
  return waitFor(() => {
    const control = screen.getByText(label).closest('leo-button')
    const button = control?.shadowRoot?.querySelector('button')
    expect(button).toBeInstanceOf(HTMLButtonElement)
    return button!
  })
}

describe('ToolPermissionChallenge', () => {
  it('renders the supplied implications with markdown emphasis', () => {
    renderChallenge(
      toolUseEvent(
        Mojom.TAB_MANAGEMENT_TOOL_NAME,
        'Send **URL**, **Title** and **short excerpts of the page text**.',
      ),
    )

    for (const text of ['URL', 'Title', 'short excerpts of the page text']) {
      expect(screen.getByText(text).tagName).toBe('STRONG')
    }
  })

  it.each([
    Mojom.TAB_MANAGEMENT_TOOL_NAME,
    Mojom.SEMANTIC_HISTORY_SEARCH_TOOL_NAME,
  ])(
    'does not invent implications for %s when the tool supplied none',
    (name) => {
      const { container } = renderChallenge(toolUseEvent(name))
      expect(container).not.toHaveTextContent('CHAT_UI_TOOL_')
    },
  )

  it('shows implications from tools without a frontend-specific name', () => {
    renderChallenge(toolUseEvent('website_tool', 'Only share **this page**.'))
    expect(screen.getByText('this page').tagName).toBe('STRONG')
  })

  it.each<[string, Mojom.PermissionChallengeDecision]>([
    [
      S.CHAT_UI_PERMISSION_CHALLENGE_ALLOW_BUTTON,
      Mojom.PermissionChallengeDecision.kAllowOnce,
    ],
    [
      S.CHAT_UI_WEBSITE_TOOL_PERMISSION_ALLOW_SESSION,
      Mojom.PermissionChallengeDecision.kAllowSession,
    ],
    [
      S.CHAT_UI_PERMISSION_CHALLENGE_DENY_BUTTON,
      Mojom.PermissionChallengeDecision.kDeny,
    ],
  ])('forwards %s with the exact tool-use ID', async (label, decision) => {
    renderChallenge(toolUseEvent('website_tool', 'Only this page.', true))
    fireEvent.click(await decisionButton(label))
    expect(mockProcessPermissionChallenge).toHaveBeenCalledTimes(1)
    expect(mockProcessPermissionChallenge).toHaveBeenCalledWith(
      'tool-use-id',
      decision,
    )
  })

  it('does not offer a session grant when the tool does not support it', () => {
    renderChallenge(toolUseEvent('website_tool'))
    expect(
      screen.queryByText(S.CHAT_UI_WEBSITE_TOOL_PERMISSION_ALLOW_SESSION),
    ).not.toBeInTheDocument()
  })

  it('does not submit a decision when the challenge is not interactive', async () => {
    renderChallenge(toolUseEvent('website_tool'), false)
    const allow = await decisionButton(
      S.CHAT_UI_PERMISSION_CHALLENGE_ALLOW_BUTTON,
    )
    expect(allow).toBeDisabled()
    fireEvent.click(allow)
    expect(mockProcessPermissionChallenge).not.toHaveBeenCalled()
  })

  it('renders nothing without a permission challenge', () => {
    const event = toolUseEvent(Mojom.TAB_MANAGEMENT_TOOL_NAME)
    event.permissionChallenge = undefined
    expect(renderChallenge(event).container).toBeEmptyDOMElement()
  })
})
