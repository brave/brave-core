// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { render, waitFor, act } from '@testing-library/react'
import { RegenerateAnswerMenu } from '.'
import '@testing-library/jest-dom'

const mockModels = [
  {
    key: '1',
    displayName: 'Model One',
    visionSupport: false,
    options: {
      leoModelOptions: {
        name: 'model-one',
        displayMaker: 'Company',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: '2',
    displayName: 'Model Two',
    visionSupport: true,
    options: {
      leoModelOptions: {
        name: 'model-two-premium',
        displayMaker: 'Company',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
]

describe('regenerate answer menu', () => {
  it('ensures the menu opens and is visible and closes.', async () => {
    const TestComponent = () => {
      const [isOpen, setIsOpen] = React.useState(false)
      return (
        <RegenerateAnswerMenu
          isOpen={isOpen}
          onOpen={() => setIsOpen(true)}
          onClose={() => setIsOpen(false)}
          onRegenerate={() => {}}
          leoModels={mockModels}
          turnModelKey='1'
        />
      )
    }

    render(<TestComponent />)

    // Make sure the anchor button is visible
    const anchorButton: any = document.querySelector('leo-button')
    expect(anchorButton).toBeInTheDocument()
    expect(anchorButton).toBeVisible()

    // Click the anchor button to open the menu
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button').click()
    })

    const menu: any = await waitFor(
      () => document.querySelector('leo-buttonmenu')!,
    )

    // Covers that onOpen was called
    expect(menu).toHaveAttribute('isOpen', 'true')

    // Make sure the menu is open
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveTextContent('regenerateAnswerMenuTitle')
    expect(menu).toHaveTextContent('regenerateAnswerButtonLabel')
    expect(menu).toHaveTextContent('Model One')
    expect(menu).toHaveTextContent('Model Two')

    // Click the anchor button to close the menu
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button').click()
    })
    // Covers that onClose was called
    expect(menu).toHaveAttribute('isOpen', 'false')
  })

  it('switches models and regenerates the answer', async () => {
    const onRegenerateMock = jest.fn()
    render(
      <RegenerateAnswerMenu
        isOpen={false}
        onOpen={() => {}}
        onClose={() => {}}
        onRegenerate={onRegenerateMock}
        leoModels={mockModels}
        turnModelKey='1'
      />,
    )

    const anchorButton: any = document.querySelector('leo-button')
    anchorButton?.shadowRoot?.querySelector('button').click()

    // Make sure the menu is open
    const menu: any = await waitFor(
      () => document.querySelector('leo-buttonmenu')!,
    )
    expect(menu).toBeInTheDocument()

    // Make sure the model one option is selected
    const modelOneOption: any = document.querySelector(
      'leo-menu-item[data-key="1"]',
    )
    expect(modelOneOption).toBeInTheDocument()
    expect(modelOneOption).toHaveAttribute('aria-selected', 'true')

    // Make sure the model two option is not selected
    const modelTwoOption: any = document.querySelector(
      'leo-menu-item[data-key="2"]',
    )
    expect(modelTwoOption).toBeInTheDocument()
    expect(modelTwoOption).not.toHaveAttribute('aria-selected')

    // Select the model two option
    await act(async () => {
      modelTwoOption.click()
    })

    await waitFor(() => {
      expect(modelOneOption).not.toHaveAttribute('aria-selected')
      expect(modelTwoOption).toHaveAttribute('aria-selected', 'true')
    })

    // Click the regenerate button and make sure the onRegenerate
    // function is called with model two
    const regenerateButton: any = document.querySelector(
      'leo-menu-item[data-key="regenerate"]',
    )
    expect(regenerateButton).toBeInTheDocument()
    await act(async () => {
      regenerateButton.click()
    })
    expect(onRegenerateMock).toHaveBeenCalledWith('2')
  })

  it('handles invalid turnModelKey gracefully', async () => {
    const onRegenerateMock = jest.fn()
    render(
      <RegenerateAnswerMenu
        isOpen={false}
        onOpen={() => {}}
        onClose={() => {}}
        onRegenerate={onRegenerateMock}
        leoModels={mockModels}
        turnModelKey='3'
      />,
    )

    const anchorButton: any = document.querySelector('leo-button')
    anchorButton?.shadowRoot?.querySelector('button').click()

    // Make sure the menu is open
    const menu: any = await waitFor(
      () => document.querySelector('leo-buttonmenu')!,
    )
    expect(menu).toBeInTheDocument()

    // Verify no model is selected
    const modelOneOption: any = document.querySelector(
      'leo-menu-item[data-key="1"]',
    )
    const modelTwoOption: any = document.querySelector(
      'leo-menu-item[data-key="2"]',
    )
    expect(modelOneOption).not.toHaveAttribute('aria-selected')
    expect(modelTwoOption).not.toHaveAttribute('aria-selected')

    // Click the regenerate button and make sure the onRegenerate
    // function is not called since the turnModelKey is not in
    // the models list
    const regenerateButton: any = document.querySelector(
      'leo-menu-item[data-key="regenerate"]',
    )
    expect(regenerateButton).toBeInTheDocument()
    await act(async () => {
      regenerateButton.click()
    })
    expect(onRegenerateMock).not.toHaveBeenCalled()

    // Select a valid model
    await act(async () => {
      modelOneOption.click()
    })

    await waitFor(() => {
      expect(modelOneOption).toHaveAttribute('aria-selected', 'true')
    })

    // Click the regenerate button again and make sure the onRegenerate
    // function is called with model one
    await act(async () => {
      regenerateButton.click()
    })
    expect(onRegenerateMock).toHaveBeenCalledWith('1')
  })
})
