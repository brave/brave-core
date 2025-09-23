// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import SmartModeDialog from './index'
import { AIChatReactContext } from '../../state/ai_chat_context'
import { ConversationReactContext } from '../../state/conversation_context'
import * as Mojom from '../../../common/mojom'

// Mock locale function
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    const mockStrings: { [key: string]: string } = {
      CHAT_UI_EDIT_SMART_MODE_TITLE: 'Edit Smart Mode',
      CHAT_UI_NEW_SMART_MODE_TITLE: 'New Smart Mode',
      CHAT_UI_SMART_MODE_DESCRIPTION: 'Create a custom shortcut',
      CHAT_UI_MODEL_FOR_PROMPT_LABEL: 'Model for prompt',
      CHAT_UI_USE_DEFAULT_MODEL_LABEL: 'Use default model',
      CHAT_UI_SHORTCUT_LABEL: 'Shortcut',
      CHAT_UI_PROMPT_LABEL: 'Prompt',
      CHAT_UI_SAVE_BUTTON_LABEL: 'Save',
      CHAT_UI_CANCEL_BUTTON_LABEL: 'Cancel',
      CHAT_UI_DELETE_BUTTON_LABEL: 'Delete',
      CHAT_UI_DELETE_SMART_MODE_TITLE: 'Delete Smart Mode',
      CHAT_UI_DELETE_SMART_MODE_WARNING: 'Are you sure?',
      CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM: 'Premium',
      CHAT_UI_MODEL_LOCAL_LABEL: 'Local',
    }
    return mockStrings[key] || key
  },
}))

// Mock styles
jest.mock('./style.module.scss', () => ({
  formSection: 'formSection',
  dropdown: 'dropdown',
  dropdownValue: 'dropdownValue',
  gradientIcon: 'gradientIcon',
  optionContent: 'optionContent',
  optionLeft: 'optionLeft',
  optionLabel: 'optionLabel',
  footer: 'footer',
  leftButton: 'leftButton',
  rightButtons: 'rightButtons',
  deleteButton: 'deleteButton',
}))

// Mock getModelIcon
jest.mock('../../../common/constants', () => ({
  getModelIcon: (modelKey: string) => `icon-${modelKey}`,
}))

describe('SmartModeDialog', () => {
  const mockModels: Mojom.Model[] = [
    {
      key: 'claude-3-haiku',
      displayName: 'Claude 3 Haiku',
      options: {
        leoModelOptions: { access: Mojom.ModelAccess.BASIC },
      },
    },
    {
      key: 'claude-3-sonnet',
      displayName: 'Claude 3 Sonnet',
      options: {
        leoModelOptions: { access: Mojom.ModelAccess.PREMIUM },
      },
    },
    {
      key: 'local-model',
      displayName: 'Local Model',
      options: {
        customModelOptions: { endpoint: 'localhost' },
      },
    },
  ]

  const mockExistingSmartMode: Mojom.SmartMode = {
    id: 'existing-mode',
    shortcut: 'existing',
    prompt: 'Existing prompt',
    model: 'claude-3-haiku',
  }

  const defaultAIChatContext = {
    smartModes: [mockExistingSmartMode],
    smartModeDialog: null,
    setSmartModeDialog: jest.fn(),
    service: {
      createSmartMode: jest.fn(),
      updateSmartMode: jest.fn(),
      deleteSmartMode: jest.fn(),
    },
  }

  const defaultConversationContext = {
    allModels: mockModels,
  }

  const renderDialog = (aiChatOverrides = {}, conversationOverrides = {}) => {
    const aiChatContext = { ...defaultAIChatContext, ...aiChatOverrides }
    const conversationContext = {
      ...defaultConversationContext,
      ...conversationOverrides,
    }

    return render(
      <AIChatReactContext.Provider value={aiChatContext as any}>
        <ConversationReactContext.Provider value={conversationContext as any}>
          <SmartModeDialog />
        </ConversationReactContext.Provider>
      </AIChatReactContext.Provider>,
    )
  }

  beforeEach(() => {
    jest.clearAllMocks()
  })

  describe('Dialog visibility', () => {
    it('should not render dialog when smartModeDialog is null', () => {
      renderDialog()
      // When smartModeDialog is null, no dialog should be visible
      const dialogs = document.querySelectorAll('leo-dialog')
      dialogs.forEach((dialog) => {
        expect(dialog).not.toHaveAttribute('isOpen', 'true')
      })
    })

    it('should render new smart mode dialog when set', () => {
      renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
      })
      expect(screen.getByText('New Smart Mode')).toBeInTheDocument()
      expect(screen.getByText('Create a custom shortcut')).toBeInTheDocument()
    })

    it('should render edit smart mode dialog when existing mode is set', () => {
      renderDialog({
        smartModeDialog: mockExistingSmartMode,
      })
      expect(screen.getByText('Edit Smart Mode')).toBeInTheDocument()
    })
  })

  describe('Form labels and structure', () => {
    it('should show form labels for new mode', () => {
      renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
      })

      expect(screen.getByText('Model for prompt')).toBeInTheDocument()
      expect(screen.getByText('Shortcut')).toBeInTheDocument()
      expect(screen.getByText('Prompt')).toBeInTheDocument()
      expect(screen.getByText('Save')).toBeInTheDocument()
      // Main dialog and delete confirmation both have Cancel buttons
      expect(screen.getAllByText('Cancel')).toHaveLength(2)
    })

    it('should show form labels for edit mode', () => {
      renderDialog({
        smartModeDialog: mockExistingSmartMode,
      })

      expect(screen.getByText('Model for prompt')).toBeInTheDocument()
      expect(screen.getByText('Shortcut')).toBeInTheDocument()
      expect(screen.getByText('Prompt')).toBeInTheDocument()
      expect(screen.getByText('Save')).toBeInTheDocument()
      // Main dialog and delete confirmation both have Cancel buttons
      expect(screen.getAllByText('Cancel')).toHaveLength(2)
    })
  })

  describe('Model selection', () => {
    it('should render model dropdown with all available models', () => {
      renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
      })

      expect(screen.getByText('Claude 3 Haiku')).toBeInTheDocument()
      expect(screen.getByText('Claude 3 Sonnet')).toBeInTheDocument()
      expect(screen.getByText('Local Model')).toBeInTheDocument()
      // Appears in dropdown value and as option
      expect(screen.getAllByText('Use default model')).toHaveLength(2)
    })

    it('should show premium label for premium models', () => {
      renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
      })

      expect(screen.getByText('Premium')).toBeInTheDocument()
    })

    it('should show local label for custom models', () => {
      renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
      })

      expect(screen.getByText('Local')).toBeInTheDocument()
    })
  })

  describe('Button interactions', () => {
    it('should call setSmartModeDialog(null) when cancel is clicked', () => {
      const mockSetSmartModeDialog = jest.fn()
      renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
        setSmartModeDialog: mockSetSmartModeDialog,
      })

      const cancelButtons = screen.getAllByText('Cancel')
      fireEvent.click(cancelButtons[0]) // Click the main dialog cancel button

      expect(mockSetSmartModeDialog).toHaveBeenCalledWith(null)
    })

    it('should show delete button only in edit mode', () => {
      // New mode - should not have visible delete button in leftButton area
      const { rerender } = renderDialog({
        smartModeDialog: { id: undefined, shortcut: '', prompt: '', model: '' },
      })
      const leftButtonArea = document.querySelector('.leftButton')
      expect(leftButtonArea).toBeEmptyDOMElement()

      // Edit mode - should have delete button
      rerender(
        <AIChatReactContext.Provider
          value={
            {
              ...defaultAIChatContext,
              smartModeDialog: mockExistingSmartMode,
            } as any
          }
        >
          <ConversationReactContext.Provider
            value={defaultConversationContext as any}
          >
            <SmartModeDialog />
          </ConversationReactContext.Provider>
        </AIChatReactContext.Provider>,
      )
      // Main delete button + confirmation delete button
      expect(screen.getAllByText('Delete')).toHaveLength(2)
    })

    it('should show delete confirmation dialog when delete is clicked', async () => {
      const mockDeleteSmartMode = jest.fn().mockResolvedValue({})
      renderDialog({
        smartModeDialog: mockExistingSmartMode,
        service: {
          ...defaultAIChatContext.service,
          deleteSmartMode: mockDeleteSmartMode,
        },
      })

      const deleteButtons = screen.getAllByText('Delete')
      fireEvent.click(deleteButtons[0]) // Click the main delete button

      expect(screen.getByText('Delete Smart Mode')).toBeInTheDocument()
      expect(screen.getByText('Are you sure?')).toBeInTheDocument()
      // Main dialog + confirmation dialog
      expect(screen.getAllByText('Cancel')).toHaveLength(2)
      // Delete button + confirm button
      expect(screen.getAllByText('Delete')).toHaveLength(2)

      // Click confirm delete
      const confirmButtons = screen.getAllByText('Delete')
      const confirmDeleteButton = confirmButtons[1]
      fireEvent.click(confirmDeleteButton)

      await waitFor(() => {
        expect(mockDeleteSmartMode).toHaveBeenCalledWith('existing-mode')
      })
    })
  })
})
