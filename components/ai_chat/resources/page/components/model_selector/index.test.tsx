// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { render, act, within } from '@testing-library/react'
import { ModelSelector } from '.'
import '@testing-library/jest-dom'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'

// Mock the contexts
jest.mock('../../state/ai_chat_context', () => ({
  useAIChat: jest.fn(),
}))

jest.mock('../../state/conversation_context', () => ({
  useConversation: jest.fn(),
}))

describe('ModelSelector', () => {
  const mockUseAIChat = useAIChat as jest.MockedFunction<typeof useAIChat>
  const mockUseConversation = useConversation as jest.MockedFunction<
    typeof useConversation
  >

  const mockModels = [
    {
      key: 'chat-automatic',
      displayName: 'Automatic',
      isSuggestedModel: true,
      isNearModel: false,
      supportsTools: true,
      options: {
        leoModelOptions: {
          access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        },
      },
    },
    {
      key: 'chat-basic',
      displayName: 'Basic Model',
      isSuggestedModel: true,
      isNearModel: false,
      supportsTools: true,
      options: {
        leoModelOptions: {
          access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        },
      },
    },
    {
      key: 'another-chat-basic',
      displayName: 'Another Basic Model',
      isSuggestedModel: false,
      isNearModel: false,
      supportsTools: false,
      options: {
        leoModelOptions: {
          access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        },
      },
    },
    {
      key: 'chat-premium',
      displayName: 'Premium Model',
      isSuggestedModel: true,
      isNearModel: true,
      supportsTools: true,
      options: {
        leoModelOptions: {
          access: Mojom.ModelAccess.PREMIUM,
        },
      },
    },
    {
      key: 'another-chat-premium',
      displayName: 'Another Premium Model',
      isSuggestedModel: false,
      isNearModel: false,
      supportsTools: false,
      options: {
        leoModelOptions: {
          access: Mojom.ModelAccess.PREMIUM,
        },
      },
    },

    {
      key: 'chat-custom',
      displayName: 'Custom Model',
      isNearModel: false,
      options: {
        customModelOptions: {
          modelRequestName: 'Custom Model Request',
          endpoint: {
            url: 'http://localhost:8080',
          },
        },
      },
    },
  ]

  const defaultAIChatContext = {
    isPremiumUser: false,
  }

  const defaultConversationContext = {
    allModels: mockModels,
    currentModel: mockModels[1],
    setCurrentModel: jest.fn(),
  }

  beforeEach(() => {
    jest.clearAllMocks()
    mockUseAIChat.mockReturnValue(defaultAIChatContext as any)
    mockUseConversation.mockReturnValue(defaultConversationContext as any)
  })

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

  const getShowAllModelsButton = () => {
    const showAllModelsButton = document.querySelector<HTMLElement>(
      'leo-menu-item[data-testid="show-all-models-button"]',
    )
    expect(showAllModelsButton).toBeInTheDocument()
    expect(showAllModelsButton).toBeVisible()
    return showAllModelsButton
  }

  it('renders the component with the current model', async () => {
    render(<ModelSelector />)

    // Make sure the anchor button is visible
    const anchorButton = getAnchorButton()
    expect(anchorButton).toHaveTextContent('Basic Model')

    // Make sure the menu is visible
    const menu = getMenu()
    expect(menu).toHaveAttribute('isOpen', 'false')
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button')?.click()
    })
    expect(menu).toHaveAttribute('isOpen', 'true')

    // Make sure the default menu items are visible
    const menuItems = document.querySelectorAll<HTMLElement>('leo-menu-item')
    expect(menuItems).toHaveLength(4)
    expect(menuItems[0]).toHaveTextContent('Automatic')
    expect(menuItems[1]).toHaveTextContent('Basic Model')
    expect(menuItems[2]).toHaveTextContent('Premium Model')
  })

  it('shows all models if Show all models button is clicked', async () => {
    render(<ModelSelector />)

    // Click the anchor button to show menu
    const anchorButton = getAnchorButton()
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button')?.click()
    })

    // Test Show all models button
    const showAllModelsButton = getShowAllModelsButton()
    expect(showAllModelsButton).toHaveTextContent(
      'CHAT_UI_SHOW_ALL_MODELS_BUTTON',
    )

    // Click the show all models button
    await act(async () => {
      showAllModelsButton?.click()
    })

    // Check that the button text has changed
    expect(showAllModelsButton).toHaveTextContent(
      'CHAT_UI_RECOMMENDED_MODELS_BUTTON',
    )

    // Check that all model items are visible
    const allMenuItems = document.querySelectorAll<HTMLElement>('leo-menu-item')
    expect(allMenuItems).toHaveLength(7)
    expect(allMenuItems[0]).toHaveTextContent('Automatic')
    expect(allMenuItems[1]).toHaveTextContent('Basic Model')
    expect(allMenuItems[2]).toHaveTextContent('Another Basic Model')
    expect(allMenuItems[3]).toHaveTextContent('Premium Model')
    expect(allMenuItems[4]).toHaveTextContent('Another Premium Model')
    expect(allMenuItems[5]).toHaveTextContent('Custom Model')

    const labels = document.querySelectorAll<HTMLElement>('leo-label')

    // Check that current model label is visible
    expect(labels[0]).toBeInTheDocument()
    expect(labels[0]).toBeVisible()
    expect(labels[0]).toHaveTextContent('CHAT_UI_CURRENT_LABEL')

    // Check that premium model label is visible
    expect(labels[2]).toBeInTheDocument()
    expect(labels[2]).toBeVisible()
    expect(labels[2]).toHaveTextContent(
      'CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM',
    )

    // Check that local label is visible
    expect(labels[3]).toBeInTheDocument()
    expect(labels[3]).toBeVisible()
    expect(labels[3]).toHaveTextContent('CHAT_UI_MODEL_LOCAL_LABEL')
  })

  it('should call setCurrentModel when a model is clicked', async () => {
    const mockSetCurrentModel = jest.fn()
    mockUseConversation.mockReturnValue({
      ...defaultConversationContext,
      setCurrentModel: mockSetCurrentModel,
    } as any)

    render(<ModelSelector />)

    // Click the anchor button to show menu
    const anchorButton = getAnchorButton()
    expect(anchorButton).toHaveTextContent('Basic Model')
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button')?.click()
    })

    // Make sure the menu is open
    const menu = getMenu()
    expect(menu).toHaveAttribute('isOpen', 'true')

    // Click the show all models button
    const showAllModelsButton = getShowAllModelsButton()
    await act(async () => {
      showAllModelsButton?.click()
    })

    // Select another model
    const allMenuItems = document.querySelectorAll<HTMLElement>('leo-menu-item')
    expect(allMenuItems).toHaveLength(7)
    await act(async () => {
      allMenuItems[1].click()
    })

    // Verify setCurrentModel was called with the premium model
    expect(mockSetCurrentModel).toHaveBeenCalledWith(mockModels[1])
    expect(menu).toHaveAttribute('isOpen', 'false')
  })

  it(
    'should only show compatible models in the model selector if in'
      + 'agent mode',
    async () => {
      mockUseAIChat.mockReturnValue({
        ...defaultAIChatContext,
        isAIChatAgentProfileFeatureEnabled: true,
        isAIChatAgentProfile: true,
      } as any)

      // Simulate the real context behavior: in agent mode,
      // filter models by supportsTools. This mimics the logic
      // in Conversation Context.
      const filteredModels = mockModels.filter(
        (model) => model.supportsTools === true,
      )
      mockUseConversation.mockReturnValue({
        ...defaultConversationContext,
        allModels: filteredModels,
      } as any)

      render(<ModelSelector />)

      // Click the anchor button to show menu
      const anchorButton = getAnchorButton()
      await act(async () => {
        anchorButton?.shadowRoot?.querySelector('button')?.click()
      })

      // Test Show all models button
      const showAllModelsButton = getShowAllModelsButton()
      expect(showAllModelsButton).toHaveTextContent(
        'CHAT_UI_SHOW_ALL_MODELS_BUTTON',
      )

      // Click the show all models button
      await act(async () => {
        showAllModelsButton?.click()
      })

      // Check that all model items are visible
      const allMenuItems =
        document.querySelectorAll<HTMLElement>('leo-menu-item')
      expect(allMenuItems).toHaveLength(4)
      expect(allMenuItems[0]).toHaveTextContent('Automatic')
      expect(allMenuItems[1]).toHaveTextContent('Basic Model')
      expect(allMenuItems[2]).toHaveTextContent('Premium Model')

      // Check that the AI Agent profile alert is visible
      const alert = document.querySelector<HTMLElement>('leo-alert')
      expect(alert).toBeInTheDocument()
      expect(alert).toBeVisible()
      expect(alert).toHaveTextContent('CHAT_UI_AGENT_MODE_MODEL_INFO')
    },
  )
  it('should display near icon and beta label for near models', async () => {
    render(<ModelSelector />)

    // Make sure the anchor button is visible
    const anchorButton = getAnchorButton()
    expect(anchorButton).toHaveTextContent('Basic Model')

    // Make sure the menu is visible
    const menu = getMenu()
    expect(menu).toHaveAttribute('isOpen', 'false')
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button')?.click()
    })
    expect(menu).toHaveAttribute('isOpen', 'true')

    // Make sure the default menu items are visible
    const menuItems = document.querySelectorAll<HTMLElement>('leo-menu-item')
    const nearMenuItem = menuItems[2]

    // Check that the beta label is visible
    expect(nearMenuItem).toHaveTextContent('CHAT_UI_MODEL_BETA_LABEL')

    // Check that the near icon is present in the Premium Model menu item
    const nearIcon = within(nearMenuItem).getByRole('img')
    expect(nearIcon).toHaveClass('nearIcon')
  })
})
