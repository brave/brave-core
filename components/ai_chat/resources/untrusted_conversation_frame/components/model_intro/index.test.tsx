/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from '@testing-library/react'
import '@testing-library/jest-dom'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import { clearAllDataForTesting } from '$web-common/api'
import ModelIntro from '.'

describe('ModelIntro', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  const currentModel: Mojom.Model = {
    key: 'test-model',
    displayName: 'Test Model',
    visionSupport: false,
    isSuggestedModel: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    isNearModel: false,
    options: {
      customModelOptions: undefined,
      leoModelOptions: {
        displayMaker: 'Test Maker',
        name: 'test-leo-model',
        access: Mojom.ModelAccess.BASIC,
        longConversationWarningCharacterLimit: 1,
        maxAssociatedContentLength: 2,
        category: Mojom.ModelCategory.CHAT,
      },
    },
  }

  it('should render model intro', () => {
    const { container } = render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            allModels: [currentModel],
            currentModelKey: currentModel.key,
            isLeoModel: true,
          },
        }}
      >
        <ModelIntro />
      </MockContext>,
    )
    expect(container).toBeInTheDocument()

    // Test that the model text is rendered
    const modelText = container.querySelector<HTMLHeadingElement>('h3')
    expect(modelText).toBeInTheDocument()
    expect(modelText).toHaveTextContent('Test Model')

    // Test that the model intro icon is rendered
    const modelIntroIcon = container.querySelector<HTMLDivElement>('leo-icon')
    expect(modelIntroIcon).toBeInTheDocument()
    expect(modelIntroIcon).toHaveAttribute('name', 'product-brave-leo')

    // Test that the tooltip is rendered
    const tooltip = container.querySelector<HTMLDivElement>('leo-tooltip')
    expect(tooltip).toBeInTheDocument()
  })

  it('should render model intro tooltip', () => {
    const { container } = render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            allModels: [currentModel],
            currentModelKey: currentModel.key,
            isLeoModel: true,
          },
        }}
      >
        <ModelIntro />
      </MockContext>,
    )
    const tooltip = container.querySelector<HTMLDivElement>('leo-tooltip')
    expect(tooltip).toBeInTheDocument()

    // Test that the tooltip content is rendered
    const tooltipContent =
      tooltip?.querySelector<HTMLDivElement>('[slot="content"]')
    expect(tooltipContent).toBeInTheDocument()

    // Test that the tooltip content has the correct message
    const tooltipContentText = tooltipContent?.textContent
    expect(tooltipContentText).toBe(S.CHAT_UI_INTRO_MESSAGE_TEST_MODEL)
  })
})
