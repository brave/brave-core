// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, act, waitFor } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import { BRAVE_SUMMARY_MODEL_KEY } from '../../../common/constants'
import ContextActionsAssistant from '.'
import MockContext from '../../mock_untrusted_conversation_context'
import '@testing-library/jest-dom'

const mockModels: Mojom.Model[] = [
  {
    key: 'model-one',
    displayName: 'Model One',
    visionSupport: false,
    supportsTools: false,
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

function renderAssistant(isSummaryResponse: boolean) {
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
  render(
    <MockContext
      allModels={mockModels}
      isPremiumUser={false}
      conversationHistory={conversationHistory}
    >
      <ContextActionsAssistant
        turnUuid='turn-uuid'
        turnModelKey='model-one'
      />
    </MockContext>,
  )
}

describe('ContextActionsAssistant', () => {
  it('hides summary model in regenerate dropdown for non-summary requests', async () => {
    renderAssistant(false)

    const anchorButton = await getRegenerateAnchorButton()
    await clickRegenerateAnchor(anchorButton)
    await getOpenMenu()

    const summaryOption = document.querySelector<HTMLElement>(
      `leo-menu-item[data-key="${BRAVE_SUMMARY_MODEL_KEY}"]`,
    )
    expect(summaryOption).toBeNull()
  })

  it('shows summary model in regenerate dropdown for summary requests', async () => {
    renderAssistant(true)

    const anchorButton = await getRegenerateAnchorButton()
    await clickRegenerateAnchor(anchorButton)
    await getOpenMenu()

    const summaryOption = document.querySelector<HTMLElement>(
      `leo-menu-item[data-key="${BRAVE_SUMMARY_MODEL_KEY}"]`,
    )
    expect(summaryOption).toBeInTheDocument()
  })
})
