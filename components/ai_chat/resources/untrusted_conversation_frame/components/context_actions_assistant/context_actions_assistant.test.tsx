// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, act, waitFor } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import { BRAVE_SUMMARY_MODEL_KEY } from '../../../common/constants'
import ContextActionsAssistant from '.'
import MockContext, {
  MockContextRef,
} from '../../mock_untrusted_conversation_context'
import { UntrustedConversationContext } from '../../untrusted_conversation_context'
import '@testing-library/jest-dom'

const mockModels: Mojom.Model[] = [
  {
    key: 'model-one',
    displayName: 'Model One',
    visionSupport: false,
    supportsTools: false,
    supportedCapabilities: [],
    isSuggestedModel: false,
    isNearModel: false,
    options: {
      leoModelOptions: {
        name: 'model-one',
        displayMaker: 'Company',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: BRAVE_SUMMARY_MODEL_KEY,
    displayName: 'Brave Summary',
    visionSupport: false,
    supportsTools: false,
    supportedCapabilities: [],
    isSuggestedModel: false,
    isNearModel: false,
    options: {
      leoModelOptions: {
        name: 'brave-summary',
        displayMaker: 'Brave',
        category: Mojom.ModelCategory.SUMMARY,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 180000,
        longConversationWarningCharacterLimit: 320000,
      },
      customModelOptions: undefined,
    },
  },
]

const getRegenerateAnchorButton = async (): Promise<HTMLButtonElement> => {
  const menu = document.querySelector('leo-buttonmenu')
  expect(menu).toBeInTheDocument()
  const anchorButton = menu?.querySelector<HTMLButtonElement>('leo-button')
  expect(anchorButton).toBeInTheDocument()
  expect(anchorButton).toBeVisible()
  return anchorButton!
}

const clickRegenerateAnchor = async (anchorButton: HTMLButtonElement) => {
  await act(async () => {
    anchorButton?.shadowRoot?.querySelector('button')?.click()
  })
}

const getOpenMenu = async () => {
  const menu = await waitFor(
    () => {
      const el = document.querySelector('leo-buttonmenu')
      if (!el || el.getAttribute('isOpen') !== 'true') {
        throw new Error('Menu not open yet')
      }
      return el
    },
    { timeout: 1000 },
  )
  expect(menu).toBeInTheDocument()
  return menu
}

const humanSummarizeTurn: Mojom.ConversationTurn = {
  characterType: Mojom.CharacterType.HUMAN,
  text: 'Summarize',
  actionType: Mojom.ActionType.SUMMARIZE_PAGE,
  events: [],
  createdTime: { internalValue: BigInt(0) },
  edits: [],
  fromBraveSearchSERP: false,
  uploadedFiles: [],
  uuid: 'human-uuid',
  prompt: undefined,
  selectedText: undefined,
  modelKey: undefined,
  skill: undefined,
  nearVerificationStatus: undefined,
}

const assistantSummaryTurn: Mojom.ConversationTurn = {
  characterType: Mojom.CharacterType.ASSISTANT,
  text: 'Summary',
  actionType: Mojom.ActionType.RESPONSE,
  events: [],
  createdTime: { internalValue: BigInt(1) },
  edits: [],
  fromBraveSearchSERP: false,
  uploadedFiles: [],
  uuid: 'turn-uuid',
  prompt: undefined,
  selectedText: undefined,
  modelKey: 'model-one',
  skill: undefined,
  nearVerificationStatus: undefined,
}

function renderAssistant(
  isSummaryResponse: boolean,
  overrides?: Partial<UntrustedConversationContext>,
  childThreadCount?: number,
) {
  const conversationHistory: Mojom.ConversationTurn[] = isSummaryResponse
    ? [humanSummarizeTurn, assistantSummaryTurn]
    : [
        {
          ...humanSummarizeTurn,
          actionType: Mojom.ActionType.UNSPECIFIED,
          uuid: 'other-human',
        },
        { ...assistantSummaryTurn, uuid: 'turn-uuid' },
      ]
  const mockRef = React.createRef<MockContextRef>()
  const renderResult = render(
    <MockContext
      ref={mockRef}
      conversationHandler={{
        getConversationHistory(_threadUuid: string | null) {
          return Promise.resolve({ conversationHistory })
        },
      }}
      initialState={{
        conversationEntriesState: {
          allModels: mockModels,
          isPremiumUser: false,
        },
      }}
      overrides={overrides}
    >
      <ContextActionsAssistant
        firstTurnUuid='turn-uuid'
        lastTurnUuid='turn-uuid'
        firstTurnModelKey='model-one'
        onEditAnswerClicked={jest.fn()}
        childThreadCount={childThreadCount}
      />
    </MockContext>,
  )
  return { ...renderResult, mockRef }
}

describe('ContextActionsAssistant', () => {
  it('hides summary model in regenerate dropdown for non-summary requests', async () => {
    const renderResult = renderAssistant(false)

    const anchorButton = await getRegenerateAnchorButton()
    await clickRegenerateAnchor(anchorButton)
    await getOpenMenu()

    const summaryOption = renderResult.queryByTestId(BRAVE_SUMMARY_MODEL_KEY)
    expect(summaryOption).toBeNull()
  })

  it('shows summary model in regenerate dropdown for summary requests', async () => {
    const renderResult = renderAssistant(true)

    const anchorButton = await getRegenerateAnchorButton()
    await clickRegenerateAnchor(anchorButton)
    await getOpenMenu()

    await renderResult.findByTestId(BRAVE_SUMMARY_MODEL_KEY)
  })

  it('disables the edit and regenerate actions while a request is in progress', async () => {
    const renderResult = renderAssistant(false, { isGenerating: true })

    const editButton = renderResult.getByTitle(S.CHAT_UI_EDIT_BUTTON_LABEL)
    expect(editButton.getAttribute('isdisabled')).toBe('true')

    const anchorButton = await getRegenerateAnchorButton()
    expect(anchorButton.getAttribute('isdisabled')).toBe('true')
  })

  it('enables the edit and regenerate actions when no request is in progress', async () => {
    const renderResult = renderAssistant(false, { isGenerating: false })

    const editButton = renderResult.getByTitle(S.CHAT_UI_EDIT_BUTTON_LABEL)
    expect(editButton.getAttribute('isdisabled')).not.toBe('true')

    const anchorButton = await getRegenerateAnchorButton()
    expect(anchorButton.getAttribute('isdisabled')).not.toBe('true')
  })

  it('does not open the regenerate menu while a request is in progress', async () => {
    renderAssistant(false, { isGenerating: true })

    const anchorButton = await getRegenerateAnchorButton()
    await clickRegenerateAnchor(anchorButton)

    const menu = document.querySelector('leo-buttonmenu')
    expect(menu).toHaveAttribute('isOpen', 'false')
  })

  it('closes the regenerate menu when a request starts', async () => {
    const { mockRef } = renderAssistant(false)

    const anchorButton = await getRegenerateAnchorButton()
    await clickRegenerateAnchor(anchorButton)
    await getOpenMenu()

    await act(async () => {
      mockRef.current!.api.state.update({ isGenerating: true })
    })

    await waitFor(() => {
      expect(document.querySelector('leo-buttonmenu')).toHaveAttribute(
        'isOpen',
        'false',
      )
    })
  })

  it('hides thread button when threads feature is disabled', () => {
    const renderResult = renderAssistant(false, {
      isThreadsFeatureEnabled: false,
    })

    expect(
      renderResult.queryByTitle(S.CHAT_UI_CREATE_THREAD_BUTTON_LABEL),
    ).not.toBeInTheDocument()
  })

  it('shows thread button when threads feature is enabled', () => {
    const renderResult = renderAssistant(false, {
      isThreadsFeatureEnabled: true,
    })

    expect(
      renderResult.getByTitle(S.CHAT_UI_CREATE_THREAD_BUTTON_LABEL),
    ).toBeInTheDocument()
  })

  it('disables thread button when the max number of threads is reached', () => {
    const renderResult = renderAssistant(
      false,
      { isThreadsFeatureEnabled: true, maxThreadsPerOriginEntry: 2 },
      2,
    )

    expect(
      renderResult
        .getByTitle(S.CHAT_UI_CREATE_THREAD_BUTTON_LABEL)
        .getAttribute('isdisabled'),
    ).toBe('true')
  })

  it('enables thread button when below the max number of threads', () => {
    const renderResult = renderAssistant(
      false,
      { isThreadsFeatureEnabled: true, maxThreadsPerOriginEntry: 2 },
      1,
    )

    expect(
      renderResult
        .getByTitle(S.CHAT_UI_CREATE_THREAD_BUTTON_LABEL)
        .getAttribute('isdisabled'),
    ).not.toBe('true')
  })
})
