// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, act, fireEvent, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import * as Mojom from '../../../common/mojom'
import { MockContext } from '../../state/mock_context'
import { useConversation } from '../../state/conversation_context'
import { stringifyContent } from '../input_box/editable_content'
import VisionModelSwitchModal from '.'

function createModel(key: string, visionSupport: boolean): Mojom.Model {
  return {
    key,
    displayName: key,
    isSuggestedModel: true,
    isNearModel: false,
    visionSupport,
    supportsTools: true,
    supportedCapabilities: [],
    options: {
      customModelOptions: undefined,
      leoModelOptions: {
        name: key,
        displayMaker: 'Brave',
        category: Mojom.ModelCategory.CHAT,
        longConversationWarningCharacterLimit: 1,
        maxAssociatedContentLength: 2,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
      },
    },
  } as Mojom.Model
}

const MODELS = [
  createModel('text-model', false),
  createModel('vision-model', true),
]

// Shows what the composer holds.
function ComposerProbe() {
  const conversation = useConversation()
  return (
    <div data-testid='composer'>
      {stringifyContent(conversation.inputText)}:
      {conversation.pendingMessageFiles.map((f) => f.filename).join(',')}
    </div>
  )
}

async function renderModal(
  conversationHandler: Partial<Mojom.ConversationHandlerInterface>,
  isWaiting = true,
) {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(
      <MockContext
        conversationHandler={conversationHandler}
        initialState={{
          conversationState: {
            allModels: MODELS,
            currentModelKey: 'text-model',
            visionModelSwitchKey: isWaiting ? 'vision-model' : undefined,
          },
        }}
      >
        <VisionModelSwitchModal />
        <ComposerProbe />
      </MockContext>,
    )
  })
  return result!
}

function getButton(label: string) {
  return screen.getByText(label).closest('leo-button')!
}

describe('VisionModelSwitchModal', () => {
  beforeEach(() => {
    clearAllDataForTesting()
    jest.clearAllMocks()
  })

  it('is hidden while no submission waits on a vision model', async () => {
    await renderModal({}, false)
    expect(
      screen.queryByText('CHAT_UI_VISION_MODEL_SWITCH_TITLE'),
    ).not.toBeInTheDocument()
  })

  it.each([
    [
      'CHAT_UI_VISION_MODEL_SWITCH_BUTTON_LABEL',
      Mojom.VisionModelSwitchDecision.kSwitch,
    ],
    [
      'CHAT_UI_VISION_MODEL_SEND_WITHOUT_IMAGES_BUTTON_LABEL',
      Mojom.VisionModelSwitchDecision.kSendWithoutImages,
    ],
  ])('answers %s', async (label, decision) => {
    const respondToVisionModelSwitch = jest.fn()
    await renderModal({ respondToVisionModelSwitch })

    expect(
      screen.getByText('CHAT_UI_VISION_MODEL_SWITCH_TITLE'),
    ).toBeInTheDocument()
    fireEvent.click(getButton(label))

    expect(respondToVisionModelSwitch).toHaveBeenCalledWith(decision)
  })

  it('returns the submission to the composer on cancel', async () => {
    const file: Mojom.UploadedFile = {
      filename: 'image.png',
      filesize: 1,
      data: [1],
      type: Mojom.UploadedFileType.kImage,
      extractedText: undefined,
    }
    const stopGenerationAndMaybeGetHumanEntry = jest.fn(() =>
      Promise.resolve({
        humanEntry: {
          text: 'What is this?',
          uploadedFiles: [file],
        } as Mojom.ConversationTurn,
      }),
    )
    const respondToVisionModelSwitch = jest.fn()

    await renderModal({
      stopGenerationAndMaybeGetHumanEntry,
      respondToVisionModelSwitch,
    })
    await act(async () => {
      fireEvent.click(getButton('CHAT_UI_CANCEL_BUTTON_LABEL'))
    })

    await waitFor(() => {
      expect(screen.getByTestId('composer')).toHaveTextContent(
        'What is this?:image.png',
      )
    })
    expect(stopGenerationAndMaybeGetHumanEntry).toHaveBeenCalled()
    expect(respondToVisionModelSwitch).not.toHaveBeenCalled()
  })
})
