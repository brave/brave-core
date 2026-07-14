// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { render, act, waitFor } from '@testing-library/react'
import { ModelSelector } from '.'
import '@testing-library/jest-dom'
import { MockContext } from '../../state/mock_context'
import { clearAllDataForTesting } from '$web-common/api'
import {
  LOCAL_VENDOR_KEY,
  PINNED_VENDOR_KEY,
} from '../../../common/vendor_icon_map'

function withCapabilities(
  model: Mojom.Model,
  capabilities: Mojom.ModelCapability[] = [],
): Mojom.Model {
  return { ...model, capabilities }
}

describe('ModelSelector', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  const mockModels: Mojom.Model[] = [
    withCapabilities(
      {
        key: 'chat-automatic',
        displayName: 'Automatic',
        isSuggestedModel: true,
        isNearModel: false,
        visionSupport: true,
        supportsTools: true,
        supportedCapabilities: [
          Mojom.ConversationCapability.CHAT,
          Mojom.ConversationCapability.CONTENT_AGENT,
        ],
        options: {
          customModelOptions: undefined,
          leoModelOptions: {
            name: 'A model',
            displayMaker: '',
            category: Mojom.ModelCategory.CHAT,
            longConversationWarningCharacterLimit: 1,
            maxAssociatedContentLength: 2,
            access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
          },
        },
      },
      [
        Mojom.ModelCapability.SEARCH,
        Mojom.ModelCapability.VISION,
        Mojom.ModelCapability.TOOLS,
      ],
    ),
    withCapabilities(
      {
        key: 'chat-basic',
        displayName: 'Basic Model',
        isSuggestedModel: true,
        isNearModel: false,
        supportsTools: true,
        supportedCapabilities: [
          Mojom.ConversationCapability.CHAT,
          Mojom.ConversationCapability.CONTENT_AGENT,
        ],
        visionSupport: true,
        options: {
          customModelOptions: undefined,
          leoModelOptions: {
            name: 'A model',
            displayMaker: 'Anthropic',
            category: Mojom.ModelCategory.CHAT,
            longConversationWarningCharacterLimit: 1,
            maxAssociatedContentLength: 2,
            access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
          },
        },
      },
      [Mojom.ModelCapability.FAST, Mojom.ModelCapability.VISION],
    ),
    withCapabilities(
      {
        key: 'another-chat-basic',
        displayName: 'Another Basic Model',
        isSuggestedModel: false,
        isNearModel: false,
        supportsTools: false,
        supportedCapabilities: [Mojom.ConversationCapability.CHAT],
        visionSupport: true,
        options: {
          customModelOptions: undefined,
          leoModelOptions: {
            name: 'A model',
            displayMaker: 'OpenAI',
            category: Mojom.ModelCategory.CHAT,
            longConversationWarningCharacterLimit: 1,
            maxAssociatedContentLength: 2,
            access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
          },
        },
      },
      [Mojom.ModelCapability.THINKING],
    ),
    withCapabilities(
      {
        key: 'chat-premium',
        displayName: 'Premium Model',
        isSuggestedModel: true,
        isNearModel: true,
        supportsTools: true,
        supportedCapabilities: [
          Mojom.ConversationCapability.CHAT,
          Mojom.ConversationCapability.CONTENT_AGENT,
        ],
        visionSupport: true,
        options: {
          customModelOptions: undefined,
          leoModelOptions: {
            name: 'A model',
            displayMaker: 'Z.ai',
            category: Mojom.ModelCategory.CHAT,
            longConversationWarningCharacterLimit: 1,
            maxAssociatedContentLength: 2,
            access: Mojom.ModelAccess.PREMIUM,
          },
        },
      },
      [Mojom.ModelCapability.SEARCH],
    ),
    withCapabilities(
      {
        key: 'another-chat-premium',
        displayName: 'Another Premium Model',
        isSuggestedModel: false,
        isNearModel: false,
        supportsTools: false,
        supportedCapabilities: [Mojom.ConversationCapability.CHAT],
        visionSupport: true,
        options: {
          customModelOptions: undefined,
          leoModelOptions: {
            name: 'A model',
            displayMaker: 'Anthropic',
            category: Mojom.ModelCategory.CHAT,
            longConversationWarningCharacterLimit: 1,
            maxAssociatedContentLength: 2,
            access: Mojom.ModelAccess.PREMIUM,
          },
        },
      },
      [Mojom.ModelCapability.VISION, Mojom.ModelCapability.TOOLS],
    ),
    withCapabilities({
      key: 'chat-brave-summary',
      displayName: 'Brave Summary',
      isSuggestedModel: false,
      isNearModel: false,
      visionSupport: true,
      supportsTools: false,
      supportedCapabilities: [Mojom.ConversationCapability.CHAT],
      options: {
        customModelOptions: undefined,
        leoModelOptions: {
          name: 'A summary model',
          displayMaker: 'Brave',
          longConversationWarningCharacterLimit: 1,
          maxAssociatedContentLength: 2,
          category: Mojom.ModelCategory.SUMMARY,
          access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        },
      },
    }),
    withCapabilities({
      key: 'chat-custom',
      displayName: 'Custom Model',
      isNearModel: false,
      visionSupport: false,
      supportsTools: false,
      supportedCapabilities: [Mojom.ConversationCapability.CHAT],
      isSuggestedModel: false,
      options: {
        leoModelOptions: undefined,
        customModelOptions: {
          modelRequestName: 'Custom Model Request',
          contextSize: 3,
          apiKey: '',
          modelSystemPrompt: '',
          longConversationWarningCharacterLimit: 1,
          maxAssociatedContentLength: 2,
          endpoint: {
            url: 'http://localhost:8080',
          },
        },
      },
    }),
  ]

  const getAnchorButton = () => {
    const anchorButton = document.querySelector<HTMLButtonElement>('leo-button')
    expect(anchorButton).toBeInTheDocument()
    expect(anchorButton).toBeVisible()
    return anchorButton
  }

  const getMenu = () => {
    const menu = document.querySelector<HTMLDivElement>('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toBeVisible()
    return menu
  }

  const openMenu = async () => {
    const anchorButton = getAnchorButton()
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button')?.click()
    })
    const menu = getMenu()
    expect(menu).toHaveAttribute('isOpen', 'true')
    return menu
  }

  const renderModelSelector = (
    props?: Partial<React.ComponentProps<typeof MockContext>>,
  ) => {
    return render(
      <MockContext
        initialState={{
          conversationState: {
            allModels: mockModels,
            currentModelKey: 'chat-basic',
          },
          serviceState: {
            pinnedModelKeys: ['chat-basic', 'chat-premium'],
          },
          ...props?.initialState,
        }}
        aiChatOverrides={{
          isAIChatAgentProfile: false,
          isAIChatAgentProfileFeatureEnabled: false,
          ...props?.aiChatOverrides,
        }}
        conversationHandler={props?.conversationHandler}
        service={props?.service}
      >
        <ModelSelector />
      </MockContext>,
    )
  }

  it('renders the component with the current model', async () => {
    renderModelSelector()

    const anchorButton = getAnchorButton()
    expect(anchorButton).toHaveTextContent('Basic Model')

    const menu = getMenu()
    expect(menu).toHaveAttribute('isOpen', 'false')
    await openMenu()

    // Pinned view: Auto + pinned keys
    const menuItems = document.querySelectorAll<HTMLElement>(
      '.modelList leo-menu-item, [class*="modelList"] leo-menu-item',
    )
    // Fallback: all leo-menu-item that are models (exclude capability filter)
    const modelItems = Array.from(
      document.querySelectorAll<HTMLElement>('leo-menu-item[data-key]'),
    )
    expect(modelItems.length).toBeGreaterThanOrEqual(3)
    expect(modelItems[0]).toHaveTextContent('Automatic')
    expect(
      modelItems.some((item) => item.textContent?.includes('Basic Model')),
    ).toBe(true)
    expect(
      modelItems.some((item) => item.textContent?.includes('Premium Model')),
    ).toBe(true)
  })

  it('filters models by vendor from the rail', async () => {
    renderModelSelector()
    await openMenu()

    const anthropicButton = document.querySelector<HTMLButtonElement>(
      `button[data-testid="vendor-rail-Anthropic"]`,
    )
    expect(anthropicButton).toBeInTheDocument()
    await act(async () => {
      anthropicButton?.click()
    })

    await waitFor(() => {
      const modelItems = Array.from(
        document.querySelectorAll<HTMLElement>('leo-menu-item[data-key]'),
      )
      expect(modelItems.map((i) => i.getAttribute('data-key'))).toEqual(
        expect.arrayContaining(['chat-basic', 'another-chat-premium']),
      )
      expect(
        modelItems.every(
          (i) =>
            i.getAttribute('data-key') === 'chat-basic'
            || i.getAttribute('data-key') === 'another-chat-premium',
        ),
      ).toBe(true)
    })
  })

  it('filters models by search query', async () => {
    renderModelSelector()
    await openMenu()

    const search = document.querySelector('leo-input') as HTMLElement & {
      onInput?: (detail: { value: string }) => void
    }
    expect(search).toBeInTheDocument()
    await act(async () => {
      // Leo CE exposes onInput as a property; set it through that path so the
      // Svelte forward() handler receives InputEventDetail.
      search.onInput?.({ value: 'Premium' })
    })

    await waitFor(() => {
      const modelItems = Array.from(
        document.querySelectorAll<HTMLElement>('leo-menu-item[data-key]'),
      )
      expect(modelItems.length).toBeGreaterThanOrEqual(1)
      expect(modelItems.every((i) => i.textContent?.includes('Premium'))).toBe(
        true,
      )
    })
  })

  it('shows local models empty state when none exist on Local rail', async () => {
    const modelsWithoutCustom = mockModels.filter(
      (m) => !m.options.customModelOptions,
    )
    renderModelSelector({
      initialState: {
        conversationState: {
          allModels: modelsWithoutCustom,
          currentModelKey: 'chat-basic',
        },
        serviceState: {
          pinnedModelKeys: ['chat-basic'],
        },
      },
    })
    await openMenu()

    const localButton = document.querySelector<HTMLButtonElement>(
      `button[data-testid="vendor-rail-${LOCAL_VENDOR_KEY}"]`,
    )
    expect(localButton).toBeInTheDocument()
    await act(async () => {
      localButton?.click()
    })

    await waitFor(() => {
      expect(
        document.querySelector('[data-testid="model-selector-empty"]'),
      ).toHaveTextContent('CHAT_UI_LOCAL_MODELS_EMPTY')
    })
  })

  it('hides internal models from the dropdown', async () => {
    renderModelSelector()
    await openMenu()

    const menu = getMenu()
    expect(menu).not.toHaveTextContent('Brave Ocelot')
    expect(menu).not.toHaveTextContent('Brave Summary')
  })

  it('should call setCurrentModel when a model is clicked', async () => {
    const mockSetCurrentModel = jest.fn()

    renderModelSelector({
      conversationHandler: {
        changeModel: mockSetCurrentModel,
      },
    })

    await openMenu()

    const basic = document.querySelector<HTMLElement>(
      'leo-menu-item[data-testid="chat-basic"]',
    )
    expect(basic).toBeInTheDocument()
    await act(async () => {
      basic?.click()
    })

    expect(mockSetCurrentModel).toHaveBeenCalledWith(mockModels[1].key)
  })

  it('should call setModelPinned when pin is chosen from options menu', async () => {
    const setModelPinned = jest.fn()
    renderModelSelector({
      service: {
        setModelPinned,
      },
      initialState: {
        conversationState: {
          allModels: mockModels,
          currentModelKey: 'chat-basic',
        },
        serviceState: {
          pinnedModelKeys: ['chat-basic'],
        },
      },
    })

    await openMenu()

    const optionsButton = document.querySelector<HTMLElement>(
      '[data-testid="model-options-chat-basic"]',
    )
    expect(optionsButton).toBeInTheDocument()
    await act(async () => {
      optionsButton?.click()
    })

    const pinItem = document.querySelector<HTMLElement>(
      '[data-testid="pin-chat-basic"]',
    )
    expect(pinItem).toBeInTheDocument()
    await act(async () => {
      pinItem?.click()
    })

    expect(setModelPinned).toHaveBeenCalledWith('chat-basic', false)
  })

  it('should call setDefaultModelKey when set as default is chosen', async () => {
    const setDefaultModelKey = jest.fn()
    renderModelSelector({
      service: {
        setDefaultModelKey,
      },
      initialState: {
        conversationState: {
          allModels: mockModels,
          currentModelKey: 'chat-basic',
          defaultModelKey: 'chat-basic',
        },
        serviceState: {
          // Keep premium in the default pinned view so its options
          // control is in the DOM without switching vendors.
          pinnedModelKeys: ['chat-basic', 'chat-premium'],
        },
      },
    })

    await openMenu()

    const optionsButton = document.querySelector<HTMLElement>(
      '[data-testid="model-options-chat-premium"]',
    )
    expect(optionsButton).toBeInTheDocument()
    await act(async () => {
      optionsButton?.click()
    })

    const setDefaultItem = document.querySelector<HTMLElement>(
      '[data-testid="set-default-chat-premium"]',
    )
    expect(setDefaultItem).toBeInTheDocument()
    await act(async () => {
      setDefaultItem?.click()
    })

    expect(setDefaultModelKey).toHaveBeenCalledWith('chat-premium')
  })

  it(
    'should only show compatible models in the model selector if in'
      + 'agent mode',
    async () => {
      const filteredModels = mockModels.filter((model) =>
        model.supportedCapabilities.includes(
          Mojom.ConversationCapability.CONTENT_AGENT,
        ),
      )

      renderModelSelector({
        initialState: {
          conversationState: {
            allModels: filteredModels,
            currentModelKey: 'chat-basic',
          },
          serviceState: {
            pinnedModelKeys: ['chat-basic', 'chat-premium'],
          },
        },
        aiChatOverrides: {
          isAIChatAgentProfileFeatureEnabled: true,
          isAIChatAgentProfile: true,
        },
      })

      await openMenu()

      const modelItems = Array.from(
        document.querySelectorAll<HTMLElement>('leo-menu-item[data-key]'),
      )
      expect(
        modelItems.every((item) =>
          ['chat-automatic', 'chat-basic', 'chat-premium'].includes(
            item.getAttribute('data-key') ?? '',
          ),
        ),
      ).toBe(true)

      const alert = document.querySelector<HTMLElement>('leo-alert')
      expect(alert).toBeInTheDocument()
      expect(alert).toBeVisible()
      expect(alert).toHaveTextContent('CHAT_UI_AGENT_MODE_MODEL_INFO')
    },
  )

  it('should display near icon and beta label for near models', async () => {
    renderModelSelector()
    await openMenu()

    const labels = document.querySelectorAll<HTMLElement>('leo-label')
    const beta = Array.from(labels).find((l) =>
      l.textContent?.includes('CHAT_UI_MODEL_BETA_LABEL'),
    )
    expect(beta).toBeTruthy()
  })

  it('keeps pinned rail selected by default', async () => {
    renderModelSelector()
    await openMenu()
    const pinned = document.querySelector(
      `button[data-testid="vendor-rail-${PINNED_VENDOR_KEY}"]`,
    )
    expect(pinned).toHaveAttribute('aria-selected', 'true')
  })
})
