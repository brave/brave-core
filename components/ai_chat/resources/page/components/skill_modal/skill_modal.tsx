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
import styles from './skill_modal_style.module.scss'
import { ModelOption } from '../model_menu_item/model_menu_item'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { AUTOMATIC_MODEL_KEY, getModelIcon } from '../../../common/constants'
import * as Mojom from '../../../common/mojom'

export default function SkillModal() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  // State
  const [selectedModel, setSelectedModel] = React.useState(
    aiChatContext.skillDialog?.model || AUTOMATIC_MODEL_KEY,
  )
  const [shortcut, setShortcut] = React.useState(
    // Explicitly set to undefined since shortcut could be an empty string
    // which would then show an immediate error.
    aiChatContext.skillDialog?.shortcut || undefined,
  )
  const [prompt, setPrompt] = React.useState(
    // Explicitly set to undefined since prompt could be an empty string
    // which would then show an immediate error.
    aiChatContext.skillDialog?.prompt || undefined,
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
    const existingSkill = aiChatContext.skills.find(
      (skill: Mojom.Skill) =>
        skill.shortcut === shortcut.trim()
        && skill.id !== aiChatContext.skillDialog?.id,
    )
    if (existingSkill) {
      return getLocale(S.CHAT_UI_SHORTCUT_DUPLICATE_ERROR)
    }

    return ''
  }, [shortcut, aiChatContext.skillDialog?.id, aiChatContext.skills])

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
    () => !!aiChatContext.skillDialog?.id,
    [aiChatContext.skillDialog?.id],
  )

  const dialogTitle = React.useMemo(() => {
    if (showDelete) {
      return getLocale(S.CHAT_UI_DELETE_SKILL_TITLE)
    }
    if (isEditMode) {
      return getLocale(S.CHAT_UI_EDIT_SKILL_TITLE)
    }
    return getLocale(S.CHAT_UI_NEW_SKILL_TITLE)
  }, [isEditMode, showDelete])

  // Methods
  const closeAndReset = React.useCallback(() => {
    aiChatContext.setSkillDialog(null)
    setShortcut(undefined)
    setPrompt(undefined)
    setSelectedModel(AUTOMATIC_MODEL_KEY)
    setShowDelete(false)
  }, [aiChatContext.setSkillDialog])

  const onSave = React.useCallback(() => {
    if (shortcutError || promptError || !shortcut || !prompt) {
      return
    }

    if (isEditMode && aiChatContext.skillDialog?.id) {
      aiChatContext.service?.updateSkill(
        aiChatContext.skillDialog.id,
        shortcut.trim(),
        prompt.trim(),
        selectedModel,
      )
    } else {
      aiChatContext.service?.createSkill(
        shortcut.trim(),
        prompt.trim(),
        selectedModel,
      )
    }

    closeAndReset()
  }, [
    shortcutError,
    promptError,
    isEditMode,
    aiChatContext.skillDialog?.id,
    shortcut,
    prompt,
    selectedModel,
    aiChatContext.service,
    closeAndReset,
  ])

  const onDelete = React.useCallback(() => {
    if (!isEditMode || !aiChatContext.skillDialog?.id) {
      return
    }

    if (aiChatContext.service?.deleteSkill) {
      aiChatContext.service.deleteSkill(aiChatContext.skillDialog.id)
    }
    closeAndReset()
  }, [
    isEditMode,
    aiChatContext.skillDialog?.id,
    aiChatContext.service,
    closeAndReset,
  ])

  return (
    <Dialog
      isOpen={!!aiChatContext.skillDialog}
      showClose
      backdropClickCloses={false}
      onClose={closeAndReset}
    >
      <div slot='title'>{dialogTitle}</div>

      <div className={styles.description}>
        {getLocale(
          showDelete
            ? S.CHAT_UI_DELETE_SKILL_WARNING
            : S.CHAT_UI_SKILL_DESCRIPTION,
        )}
      </div>

      {!showDelete && (
        <div className={styles.formSection}>
          <Dropdown
            value={selectedModel}
            onChange={(e: { value: string }) => setSelectedModel(e.value)}
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
                  [styles.gradientIcon]: selectedModel === AUTOMATIC_MODEL_KEY,
                })}
                name={getModelIcon(selectedModel)}
              />
            }
            <div slot='value'>
              {conversationContext.allModels?.find(
                (m) => m.key === selectedModel,
              )?.displayName ?? ''}
            </div>
            {conversationContext.allModels?.map((model: Mojom.Model) => (
              <ModelOption
                key={model.key}
                model={model}
                isCurrent={model.key === selectedModel}
                showPremiumLabel={!aiChatContext.isPremiumUser}
                isDisabled={
                  !aiChatContext.isPremiumUser
                  && model.options.leoModelOptions?.access
                    === Mojom.ModelAccess.PREMIUM
                }
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
