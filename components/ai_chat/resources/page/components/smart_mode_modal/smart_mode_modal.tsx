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
import TextArea from '@brave/leo/react/textarea'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import styles from './smart_mode_modal_style.module.scss'
import { ModelOption } from '../model_menu_item/model_menu_item'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { getModelIcon } from '../../../common/constants'
import * as Mojom from '../../../common/mojom'

export default function SmartModeModal() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  // State
  const [selectedModel, setSelectedModel] = React.useState(
    aiChatContext.smartModeDialog?.model,
  )
  const [shortcut, setShortcut] = React.useState(
    // Explicitly set to undefined since shortcut could be an empty string
    // which would then show an immediate error.
    aiChatContext.smartModeDialog?.shortcut || undefined,
  )
  const [prompt, setPrompt] = React.useState(
    // Explicitly set to undefined since prompt could be an empty string
    // which would then show an immediate error.
    aiChatContext.smartModeDialog?.prompt || undefined,
  )
  const [showDelete, setShowDelete] = React.useState(false)

  // Memos
  const shortcutError = React.useMemo(() => {
    // Undefined means the user hasn't typed anything yet,
    // so we don't show an immediate error.
    if (shortcut === undefined) {
      return ''
    }
    if (!shortcut.trim()) {
      return getLocale(S.CHAT_UI_SHORTCUT_REQUIRED_ERROR)
    }
    if (shortcut.trim().length > 64) {
      return getLocale(S.CHAT_UI_SHORTCUT_TOO_LONG_ERROR)
    }
    if (!/^[a-zA-Z0-9_-]+$/.test(shortcut.trim())) {
      return getLocale(S.CHAT_UI_SHORTCUT_FORMAT_ERROR)
    }

    // Check for duplicate shortcut
    const existingSmartMode = aiChatContext.smartModes.find(
      (mode: Mojom.SmartMode) =>
        mode.shortcut === shortcut.trim()
        && mode.id !== aiChatContext.smartModeDialog?.id,
    )
    if (existingSmartMode) {
      return getLocale(S.CHAT_UI_SHORTCUT_DUPLICATE_ERROR)
    }

    return ''
  }, [shortcut, aiChatContext.smartModeDialog?.id, aiChatContext.smartModes])

  const promptError = React.useMemo(() => {
    // Undefined means the user hasn't typed anything yet,
    // so we don't show an immediate error.
    if (prompt === undefined) {
      return ''
    }
    if (!prompt.trim()) {
      return getLocale(S.CHAT_UI_PROMPT_REQUIRED_ERROR)
    }
    if (prompt.length > 5000) {
      return getLocale(S.CHAT_UI_PROMPT_TOO_LONG_ERROR)
    }
    return ''
  }, [prompt])

  // Determine if we're in edit mode
  const isEditMode = React.useMemo(
    () => !!aiChatContext.smartModeDialog?.id,
    [aiChatContext.smartModeDialog?.id],
  )

  const dialogTitle = React.useMemo(() => {
    if (showDelete) {
      return getLocale(S.CHAT_UI_DELETE_SMART_MODE_TITLE)
    }
    if (isEditMode) {
      return getLocale(S.CHAT_UI_EDIT_SMART_MODE_TITLE)
    }
    return getLocale(S.CHAT_UI_NEW_SMART_MODE_TITLE)
  }, [isEditMode, showDelete])

  // Methods
  const closeAndReset = React.useCallback(() => {
    aiChatContext.setSmartModeDialog(null)
    setShortcut(undefined)
    setPrompt(undefined)
    setSelectedModel(undefined)
    setShowDelete(false)
  }, [aiChatContext.setSmartModeDialog])

  const onSave = React.useCallback(() => {
    if (shortcutError || promptError || !shortcut || !prompt) {
      return
    }

    if (isEditMode && aiChatContext.smartModeDialog?.id) {
      aiChatContext.service?.updateSmartMode(
        aiChatContext.smartModeDialog.id,
        shortcut.trim(),
        prompt.trim(),
        selectedModel || null,
      )
    } else {
      aiChatContext.service?.createSmartMode(
        shortcut.trim(),
        prompt.trim(),
        selectedModel || null,
      )
    }

    closeAndReset()
  }, [
    shortcutError,
    promptError,
    isEditMode,
    aiChatContext.smartModeDialog?.id,
    shortcut,
    prompt,
    selectedModel,
    aiChatContext.service,
    closeAndReset,
  ])

  const onDelete = React.useCallback(() => {
    if (!isEditMode || !aiChatContext.smartModeDialog?.id) {
      return
    }

    if (aiChatContext.service?.deleteSmartMode) {
      aiChatContext.service.deleteSmartMode(aiChatContext.smartModeDialog.id)
    }
    closeAndReset()
  }, [
    isEditMode,
    aiChatContext.smartModeDialog?.id,
    aiChatContext.service,
    closeAndReset,
  ])

  return (
    <Dialog
      isOpen={!!aiChatContext.smartModeDialog}
      showClose
      backdropClickCloses={false}
      onClose={closeAndReset}
    >
      <div slot='title'>{dialogTitle}</div>

      <div className={styles.description}>
        {getLocale(
          showDelete
            ? S.CHAT_UI_DELETE_SMART_MODE_WARNING
            : S.CHAT_UI_SMART_MODE_DESCRIPTION,
        )}
      </div>

      {!showDelete && (
        <div className={styles.formSection}>
          <Dropdown
            value={selectedModel}
            onChange={(e: { value: string }) => setSelectedModel(e.value || '')}
            placeholder={getLocale(S.CHAT_UI_USE_DEFAULT_MODEL_LABEL)}
            positionStrategy='fixed'
            className={styles.dropdown}
          >
            <div slot='label'>
              {getLocale(S.CHAT_UI_MODEL_FOR_PROMPT_LABEL)}
            </div>
            {
              <Icon
                slot='left-icon'
                className={classnames({
                  [styles.gradientIcon]:
                    !selectedModel || selectedModel === 'chat-automatic',
                })}
                name={
                  selectedModel === 'chat-automatic'
                    ? 'product-brave-leo'
                    : getModelIcon(selectedModel || '')
                }
              />
            }
            <div slot='value'>
              {selectedModel
                ? conversationContext.allModels?.find(
                    (m) => m.key === selectedModel,
                  )?.displayName
                : getLocale(S.CHAT_UI_USE_DEFAULT_MODEL_LABEL)}
            </div>
            <leo-option value=''>
              <div className={styles.optionLeft}>
                <div className={styles.gradientIcon}>
                  <Icon name='product-brave-leo' />
                </div>
                {getLocale(S.CHAT_UI_USE_DEFAULT_MODEL_LABEL)}
              </div>
            </leo-option>
            {conversationContext.allModels?.map((model: Mojom.Model) => (
              <ModelOption
                key={model.key}
                model={model}
                isCurrent={model.key === selectedModel}
                showPremiumLabel={!aiChatContext.isPremiumUser}
              />
            ))}
          </Dropdown>

          <Input
            type='text'
            value={shortcut || ''}
            onInput={(e) => {
              if (e.value === undefined) {
                return
              }
              setShortcut(e.value)
            }}
            placeholder={getLocale(S.CHAT_UI_SHORTCUT_PLACEHOLDER)}
            showErrors={!!shortcutError}
          >
            <b>{getLocale(S.CHAT_UI_SHORTCUT_LABEL)}</b>
            <div slot='errors'>{shortcutError}</div>
          </Input>

          <TextArea
            value={prompt || ''}
            onInput={(e) => {
              if (e.value === undefined) {
                return
              }
              setPrompt(e.value)
            }}
            placeholder={getLocale(S.CHAT_UI_PROMPT_PLACEHOLDER)}
            rows={4}
            showErrors={!!promptError}
          >
            <b>{getLocale(S.CHAT_UI_PROMPT_LABEL)}</b>
            <div slot='errors'>{promptError}</div>
          </TextArea>
        </div>
      )}

      <div
        className={classnames({
          [styles.footer]: true,
          [styles.footerAlignRight]: showDelete || !isEditMode,
        })}
      >
        {isEditMode && !showDelete && (
          <Button
            kind='plain'
            className={styles.deleteButton}
            onClick={() => setShowDelete(true)}
          >
            {getLocale(S.CHAT_UI_DELETE_BUTTON_LABEL)}
          </Button>
        )}
        <div className={styles.rightButtons}>
          <Button
            kind='plain-faint'
            onClick={showDelete ? () => setShowDelete(false) : closeAndReset}
          >
            {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
          </Button>
          {!showDelete && (
            <Button
              kind='filled'
              onClick={onSave}
              isDisabled={
                !!shortcutError || !!promptError || !shortcut || !prompt
              }
            >
              {getLocale(S.CHAT_UI_SAVE_BUTTON_LABEL)}
            </Button>
          )}
          {showDelete && (
            <Button
              kind='filled'
              className={styles.deleteButton}
              onClick={onDelete}
            >
              {getLocale(S.CHAT_UI_DELETE_BUTTON_LABEL)}
            </Button>
          )}
        </div>
      </div>
    </Dialog>
  )
}
