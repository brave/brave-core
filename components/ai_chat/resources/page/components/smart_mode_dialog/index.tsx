/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Dropdown from '@brave/leo/react/dropdown'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'
import Label from '@brave/leo/react/label'
import TextArea from '@brave/leo/react/textarea'
import styles from './style.module.scss'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { getModelIcon } from '../../../common/constants'
import * as Mojom from '../../../common/mojom'

// Component showing dialogs for adding/editing/deleting smart modes.
export default function SmartModeDialog() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const [shortcut, setShortcut] = React.useState('')
  const [shortcutError, setShortcutError] = React.useState('')
  const [promptError, setPromptError] = React.useState('')

  const validateShortcut = (value: string): string => {
    if (!value.trim()) {
      return getLocale(S.CHAT_UI_SHORTCUT_REQUIRED_ERROR)
    }
    if (!/^[a-zA-Z0-9_-]+$/.test(value.trim())) {
      return getLocale(S.CHAT_UI_SHORTCUT_FORMAT_ERROR)
    }

    // Check for duplicate shortcut
    const existingSmartMode = aiChatContext.smartModes.find(
      (mode: Mojom.SmartMode) =>
        mode.shortcut === value.trim()
        && mode.id !== aiChatContext.smartModeDialog?.id,
    )
    if (existingSmartMode) {
      return getLocale(S.CHAT_UI_SHORTCUT_DUPLICATE_ERROR)
    }

    return ''
  }

  const validatePrompt = (value: string): string => {
    if (!value.trim()) {
      return getLocale(S.CHAT_UI_PROMPT_REQUIRED_ERROR)
    }
    if (value.length > 1000) {
      return getLocale(S.CHAT_UI_PROMPT_TOO_LONG_ERROR)
    }
    return ''
  }

  const handleShortcutChange = (value: string) => {
    setShortcut(value)
    setShortcutError(validateShortcut(value))
  }

  const handlePromptChange = (value: string) => {
    setPrompt(value)
    setPromptError(validatePrompt(value))
  }
  const [prompt, setPrompt] = React.useState('')
  const [selectedModel, setSelectedModel] = React.useState('')
  const [showDeleteConfirm, setShowDeleteConfirm] = React.useState(false)

  // Determine if we're in edit mode
  const isEditMode = !!aiChatContext.smartModeDialog?.id

  const closeAndReset = () => {
    aiChatContext.setSmartModeDialog(null)
    setShortcut('')
    setPrompt('')
    setSelectedModel('')
    setShortcutError('')
    setPromptError('')
  }

  // Initialize form when dialog opens
  React.useEffect(() => {
    if (aiChatContext.smartModeDialog) {
      const smartMode = aiChatContext.smartModeDialog
      setPrompt(smartMode.prompt || '')
      setShortcut(smartMode.shortcut || '')
      setSelectedModel(smartMode.model || '')
      setShortcutError('')
      setPromptError('')
    }
  }, [aiChatContext.smartModeDialog])

  const handleSave = async () => {
    const shortcutError = validateShortcut(shortcut)
    const promptError = validatePrompt(prompt)

    if (shortcutError || promptError) {
      setShortcutError(shortcutError)
      setPromptError(promptError)
      return
    }

    if (isEditMode && aiChatContext.smartModeDialog?.id) {
      await aiChatContext.service?.updateSmartMode(
        aiChatContext.smartModeDialog.id,
        shortcut.trim(),
        prompt.trim(),
        selectedModel || null,
      )
    } else {
      await aiChatContext.service?.createSmartMode(
        shortcut.trim(),
        prompt.trim(),
        selectedModel || null,
      )
    }

    closeAndReset()
  }

  const handleDeleteClick = () => {
    setShowDeleteConfirm(true)
  }

  const handleDeleteConfirm = async () => {
    if (!isEditMode || !aiChatContext.smartModeDialog?.id) {
      return
    }

    if (aiChatContext.service?.deleteSmartMode) {
      await aiChatContext.service.deleteSmartMode(
        aiChatContext.smartModeDialog.id,
      )
    }

    setShowDeleteConfirm(false)
    closeAndReset()
  }

  const handleDeleteCancel = () => {
    setShowDeleteConfirm(false)
  }

  const handleCancel = () => {
    closeAndReset()
  }

  return (
    <>
      <Dialog
        isOpen={!!aiChatContext.smartModeDialog}
        showClose
        onClose={handleCancel}
      >
        <div slot='title'>
          {isEditMode
            ? getLocale(S.CHAT_UI_EDIT_SMART_MODE_TITLE)
            : getLocale(S.CHAT_UI_NEW_SMART_MODE_TITLE)}
        </div>

        <div className={styles.formSection}>
          {getLocale(S.CHAT_UI_SMART_MODE_DESCRIPTION)}
        </div>

        <div className={styles.formSection}>
          <Dropdown
            value={selectedModel}
            onChange={(e: { value: string }) => setSelectedModel(e.value || '')}
            placeholder={getLocale(S.CHAT_UI_USE_DEFAULT_MODEL_LABEL)}
            className={styles.dropdown}
            positionStrategy='fixed'
          >
            <div slot='label'>
              {getLocale(S.CHAT_UI_MODEL_FOR_PROMPT_LABEL)}
            </div>
            <div
              slot='value'
              className={styles.dropdownValue}
            >
              {!selectedModel || selectedModel === 'chat-automatic' ? (
                <div className={styles.gradientIcon}>
                  <Icon
                    name={
                      selectedModel
                        ? getModelIcon(selectedModel)
                        : 'product-brave-leo'
                    }
                  />
                </div>
              ) : (
                <Icon name={getModelIcon(selectedModel)} />
              )}
              {selectedModel
                ? conversationContext.allModels?.find(
                    (m) => m.key === selectedModel,
                  )?.displayName
                : getLocale(S.CHAT_UI_USE_DEFAULT_MODEL_LABEL)}
            </div>
            <Icon
              slot='icon-after'
              name='carat-down'
            />
            <leo-option value=''>
              <div className={styles.optionContent}>
                <div className={styles.optionLeft}>
                  <div className={styles.gradientIcon}>
                    <Icon name='product-brave-leo' />
                  </div>
                  {getLocale(S.CHAT_UI_USE_DEFAULT_MODEL_LABEL)}
                </div>
              </div>
            </leo-option>
            {conversationContext.allModels?.map((model: Mojom.Model) => (
              <leo-option
                key={model.key}
                value={model.key}
              >
                <div className={styles.optionContent}>
                  <div className={styles.optionLeft}>
                    {model.key === 'chat-automatic' ? (
                      <div className={styles.gradientIcon}>
                        <Icon name={getModelIcon(model.key)} />
                      </div>
                    ) : (
                      <Icon name={getModelIcon(model.key)} />
                    )}
                    {model.displayName}
                  </div>
                  <div>
                    {model.options.leoModelOptions?.access
                      === Mojom.ModelAccess.PREMIUM && (
                      <Label
                        mode='outline'
                        color='blue'
                        className={styles.optionLabel}
                      >
                        {getLocale(S.CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM)}
                      </Label>
                    )}
                    {model.options.customModelOptions && (
                      <Label
                        mode='default'
                        color='blue'
                        className={styles.optionLabel}
                      >
                        {getLocale(S.CHAT_UI_MODEL_LOCAL_LABEL)}
                      </Label>
                    )}
                  </div>
                </div>
              </leo-option>
            ))}
          </Dropdown>
        </div>

        <div className={styles.formSection}>
          <label>{getLocale(S.CHAT_UI_SHORTCUT_LABEL)}</label>
          <Input
            type='text'
            value={shortcut}
            onChange={(e: { value: string }) => handleShortcutChange(e.value)}
            placeholder={getLocale(S.CHAT_UI_SHORTCUT_PLACEHOLDER)}
            showErrors={!!shortcutError}
          >
            <div slot='errors'>{shortcutError}</div>
          </Input>
        </div>

        <div className={styles.formSection}>
          <label>{getLocale(S.CHAT_UI_PROMPT_LABEL)}</label>
          <TextArea
            value={prompt}
            onChange={(e: { value: string }) => handlePromptChange(e.value)}
            placeholder={getLocale(S.CHAT_UI_PROMPT_PLACEHOLDER)}
            rows={4}
            showErrors={!!promptError}
          >
            <div slot='errors'>{promptError}</div>
          </TextArea>
        </div>

        <div className={styles.footer}>
          <div className={styles.leftButton}>
            {isEditMode && (
              <Button
                kind='plain'
                className={styles.deleteButton}
                onClick={handleDeleteClick}
              >
                {getLocale(S.CHAT_UI_DELETE_BUTTON_LABEL)}
              </Button>
            )}
          </div>
          <div className={styles.rightButtons}>
            <Button
              kind='plain-faint'
              onClick={handleCancel}
            >
              {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
            </Button>
            <Button
              kind='filled'
              onClick={handleSave}
              isDisabled={
                !shortcut.trim()
                || !prompt.trim()
                || !!shortcutError
                || !!promptError
              }
            >
              {getLocale(S.CHAT_UI_SAVE_BUTTON_LABEL)}
            </Button>
          </div>
        </div>
      </Dialog>

      <Dialog
        isOpen={showDeleteConfirm}
        showClose
        onClose={handleDeleteCancel}
      >
        <div slot='title'>{getLocale(S.CHAT_UI_DELETE_SMART_MODE_TITLE)}</div>

        <div>{getLocale(S.CHAT_UI_DELETE_SMART_MODE_WARNING)}</div>

        <div className={styles.footer}>
          <div className={styles.leftButton}></div>
          <div className={styles.rightButtons}>
            <Button
              kind='plain-faint'
              onClick={handleDeleteCancel}
            >
              {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
            </Button>
            <Button
              kind='filled'
              className={styles.deleteButton}
              onClick={handleDeleteConfirm}
            >
              {getLocale(S.CHAT_UI_DELETE_BUTTON_LABEL)}
            </Button>
          </div>
        </div>
      </Dialog>
    </>
  )
}
