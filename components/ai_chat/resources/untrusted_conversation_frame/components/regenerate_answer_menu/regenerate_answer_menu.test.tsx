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
    key: '2',
    displayName: 'Model Two',
    visionSupport: true,
    supportsTools: true,
    isSuggestedModel: false,
    isNearModel: false,
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
  const TestComponent = ({
    onRegenerateMock,
    turnModelKeyOverride,
  }: {
    onRegenerateMock?: jest.Mock
    turnModelKeyOverride?: string
  }) => {
    const [isOpen, setIsOpen] = React.useState(false)
    const [turnModelKey, setTurnModelKey] = React.useState(
      turnModelKeyOverride ?? '1',
    )

    const onRegenerate = React.useCallback(
      (selectedModelKey: string) => {
        act(() => setTurnModelKey(selectedModelKey))
        onRegenerateMock?.(selectedModelKey)
      },
      [onRegenerateMock],
    )

    return (
      <RegenerateAnswerMenu
        isOpen={isOpen}
        onOpen={() => act(() => setIsOpen(true))}
        onClose={() => act(() => setIsOpen(false))}
        onRegenerate={onRegenerate}
        leoModels={mockModels}
        turnModelKey={turnModelKey}
      />
    )
  }

  const getAndTestAnchorButton = async () => {
    // Make sure the anchor button is visible
    const anchorButton = document.querySelector<HTMLButtonElement>('leo-button')
    expect(anchorButton).toBeInTheDocument()
    expect(anchorButton).toBeVisible()
    return anchorButton
  }

  const clickAnchorButton = async (anchorButton: HTMLButtonElement) => {
    await act(async () => {
      anchorButton?.shadowRoot?.querySelector('button')?.click()
    })
  }

  const getAndTestMenu = async () => {
    const menu: any = await waitFor(
      () => document.querySelector('leo-buttonmenu')!,
    )
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveAttribute('isOpen', 'true')
    return menu
  }

  const getAndTestModelOption = (key: string) => {
    const modelOption = document.querySelector<HTMLElement>(
      `leo-menu-item[data-key="${key}"]`,
    )!
    expect(modelOption).toBeInTheDocument()
    return modelOption
  }

  const getAndTestRetryButton = () => {
    const retryButton = document.querySelector<HTMLElement>(
      'leo-menu-item[data-key="retrySameModel"]',
    )!
    expect(retryButton).toBeInTheDocument()
    return retryButton
  }

  it('ensures the menu opens and is visible and closes.', async () => {
    render(<TestComponent />)

    const anchorButton = (await getAndTestAnchorButton())!
    await clickAnchorButton(anchorButton)

    const menu = await getAndTestMenu()

    // Covers that onOpen was called
    expect(menu).toHaveTextContent(S.CHAT_UI_REGENERATE_ANSWER_MENU_TITLE)
    expect(menu).toHaveTextContent(S.CHAT_UI_RETRY_SAME_MODEL_BUTTON_LABEL)
    expect(menu).toHaveTextContent('Model One')
    expect(menu).toHaveTextContent('Model Two')

    await clickAnchorButton(anchorButton)
    // Covers that onClose was called
    expect(menu).toHaveAttribute('isOpen', 'false')
  })

  it('retries the answer with same model', async () => {
    const onRegenerateMock = jest.fn()
    render(<TestComponent onRegenerateMock={onRegenerateMock} />)

    const anchorButton = (await getAndTestAnchorButton())!
    await clickAnchorButton(anchorButton)

    const menu = await getAndTestMenu()

    // Make sure the model one option is selected
    const modelOneOption = getAndTestModelOption('1')
    expect(modelOneOption).toHaveAttribute('aria-selected', 'true')
    expect(modelOneOption).toHaveTextContent(S.CHAT_UI_CURRENT_LABEL)

    // Click the retry same model button and make sure the onRegenerate
    // function is called with model one
    const retrySameModelButton = getAndTestRetryButton()
    await act(async () => {
      retrySameModelButton.click()
    })
    expect(onRegenerateMock).toHaveBeenCalledWith('1')
    expect(menu).toHaveAttribute('isOpen', 'false')
  })

  it('retries the answer with a different model', async () => {
    const onRegenerateMock = jest.fn()
    render(<TestComponent onRegenerateMock={onRegenerateMock} />)

    const anchorButton = (await getAndTestAnchorButton())!
    await clickAnchorButton(anchorButton)

    const menu = await getAndTestMenu()

    // Make sure the model one option is selected
    const modelOneOption = getAndTestModelOption('1')
    expect(modelOneOption).toHaveAttribute('aria-selected', 'true')
    expect(modelOneOption).toHaveTextContent(S.CHAT_UI_CURRENT_LABEL)

    // Make sure the model two option is not selected
    const modelTwoOption = getAndTestModelOption('2')
    expect(modelTwoOption).not.toHaveAttribute('aria-selected')
    expect(modelTwoOption).not.toHaveTextContent(S.CHAT_UI_CURRENT_LABEL)

    // Select the model two option
    await act(async () => {
      modelTwoOption.click()
    })

    // Make sure the onRegenerate function is called with model two
    expect(onRegenerateMock).toHaveBeenCalledWith('2')
    expect(menu).toHaveAttribute('isOpen', 'false')

    // Reopen the menu and make sure the model two option is selected
    await clickAnchorButton(anchorButton)

    expect(menu).toBeInTheDocument()
    expect(menu).toHaveAttribute('isOpen', 'true')
    expect(modelTwoOption).toHaveAttribute('aria-selected', 'true')
    expect(modelTwoOption).toHaveTextContent(S.CHAT_UI_CURRENT_LABEL)
  })

  it('handles invalid turnModelKey gracefully', async () => {
    const onRegenerateMock = jest.fn()
    render(
      <TestComponent
        onRegenerateMock={onRegenerateMock}
        turnModelKeyOverride='3'
      />,
    )

    const anchorButton = (await getAndTestAnchorButton())!
    await clickAnchorButton(anchorButton)

    await getAndTestMenu()

    // Verify no model is selected
    const modelOneOption = getAndTestModelOption('1')
    const modelTwoOption = getAndTestModelOption('2')
    expect(modelOneOption).not.toHaveAttribute('aria-selected')
    expect(modelTwoOption).not.toHaveAttribute('aria-selected')

    // Click the regenerate button and make sure the onRegenerate
    // function is not called since the turnModelKey is not in
    // the models list
    const regenerateButton = getAndTestRetryButton()
    await act(async () => {
      regenerateButton.click()
    })
    expect(onRegenerateMock).not.toHaveBeenCalled()

    // Reopen the menu (ButtonMenu may auto-close on item click)
    await clickAnchorButton(anchorButton)
    await getAndTestMenu()

    // Select a valid model - clicking it should call onRegenerate
    const modelOneOptionRefresh = getAndTestModelOption('1')
    await act(async () => {
      modelOneOptionRefresh.click()
    })

    await waitFor(() => {
      expect(modelOneOption).toHaveAttribute('aria-selected', 'true')
    })

    // Verify onRegenerate was called when clicking the model option
    expect(onRegenerateMock).toHaveBeenCalledWith('1')
  })
})
