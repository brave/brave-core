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
import styles from './style.module.scss'

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

  const defaultModelKey = 'default-model'

  it('should render model intro when current model differs from default', () => {
    const { container } = render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            allModels: [currentModel],
            currentModelKey: currentModel.key,
            defaultModelKey,
            isLeoModel: true,
          },
        }}
      >
        <ModelIntro />
      </MockContext>,
    )
    expect(container).toBeInTheDocument()

    const modelText = container.querySelector<HTMLSpanElement>(
      `.${styles.name}`,
    )
    expect(modelText).toBeInTheDocument()
    expect(modelText).toHaveTextContent('Test Model')

    const modelIntroIcon = container.querySelector<HTMLDivElement>('leo-icon')
    expect(modelIntroIcon).toBeInTheDocument()
    expect(modelIntroIcon).toHaveAttribute('name', 'product-brave-leo')

    const tooltip = container.querySelector<HTMLDivElement>('leo-tooltip')
    expect(tooltip).toBeInTheDocument()
  })

  it('should not render when current model matches default', () => {
    const { container } = render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            allModels: [currentModel],
            currentModelKey: currentModel.key,
            defaultModelKey: currentModel.key,
            isLeoModel: true,
          },
        }}
      >
        <ModelIntro />
      </MockContext>,
    )

    expect(container.querySelector(`.${styles.name}`)).not.toBeInTheDocument()
    expect(container.querySelector('leo-tooltip')).not.toBeInTheDocument()
  })

  it('should render model intro tooltip', () => {
    const { container } = render(
      <MockContext
        initialState={{
          conversationEntriesState: {
            allModels: [currentModel],
            currentModelKey: currentModel.key,
            defaultModelKey,
            isLeoModel: true,
          },
        }}
      >
        <ModelIntro />
      </MockContext>,
    )
    const tooltip = container.querySelector<HTMLDivElement>('leo-tooltip')
    expect(tooltip).toBeInTheDocument()

    const tooltipContent =
      tooltip?.querySelector<HTMLDivElement>('[slot="content"]')
    expect(tooltipContent).toBeInTheDocument()

    const tooltipContentText = tooltipContent?.textContent
    expect(tooltipContentText).toBe(S.CHAT_UI_INTRO_MESSAGE_TEST_MODEL)
  })
})
