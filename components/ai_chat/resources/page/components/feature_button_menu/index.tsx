// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import Toggle from '@brave/leo/react/toggle'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { getModelIcon } from '../../../common/constants'
import styles from './style.module.scss'
import useHasConversationStarted from '../../hooks/useHasConversationStarted'

export interface Props {
  setIsConversationsListOpen?: (value: boolean) => unknown
}

export default function FeatureMenu(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const handleSettingsClick = () => {
    aiChatContext.uiHandler?.openAIChatSettings()
  }

  // If conversation has been started, then it has been committed
  // as a conversation with messages either in memory or persisted.
  const hasConversationStarted =
    useHasConversationStarted(conversationContext.conversationUuid)

  const customModels = conversationContext.allModels.filter(
    (model) => model.options.customModelOptions
  )
  const leoModels = conversationContext.allModels.filter(
    (model) => model.options.leoModelOptions
  )

  const handleTemporaryChatToggle = (detail: { checked: boolean }) => {
    conversationContext.setTemporary(detail.checked)
  }

  return (
    <ButtonMenu className={styles.buttonMenu}>
      <Button
        slot='anchor-content'
        title={getLocale(S.CHAT_UI_LEO_SETTINGS_TOOLTIP_LABEL)}
        fab
        kind='plain-faint'
      >
        <Icon name='more-vertical' />
      </Button>
      <div className={styles.menuSectionTitle}>
        {getLocale(S.CHAT_UI_MENU_TITLE_MODELS)}
      </div>
      {leoModels.map((model) => {
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={
              model.key === conversationContext.currentModel?.key || null
            }
            onClick={() => conversationContext.setCurrentModel(model)}
          >
            <div className={styles.menuItemWithIcon}>
              <div className={styles.modelIcon} data-key={model.key}>
                <Icon name={getModelIcon(model.key)} />
              </div>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {getLocale(`CHAT_UI_${model.key.toUpperCase().replaceAll('-', '_')}_SUBTITLE`)}
                </p>
              </div>
              {model.options.leoModelOptions?.access ===
                Mojom.ModelAccess.PREMIUM &&
                !aiChatContext.isPremiumUser && (
                  <Label
                    className={styles.modelLabel}
                    mode={'outline'}
                    color='blue'
                  >
                    {getLocale(S.CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM)}
                  </Label>
                )}
            </div>
          </leo-menu-item>
        )
      })}
      {customModels.length > 0 && (
        <>
          <div className={styles.menuSeparator} />
          <div className={styles.menuSectionCustomModel}>
            {getLocale(S.AI_CHAT_MENU_TITLE_CUSTOM_MODELS)}
          </div>
        </>
      )}
      {customModels.map((model) => {
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={
              model.key === conversationContext.currentModel?.key || null
            }
            onClick={() => conversationContext.setCurrentModel(model)}
          >
            <div className={styles.menuItemWithIcon}>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {model.options.customModelOptions?.modelRequestName}
                </p>
              </div>
            </div>
          </leo-menu-item>
        )
      })}
      <div className={styles.menuSeparator} />

      {!hasConversationStarted && (
        <leo-menu-item
          data-is-interactive="true"
          onClick={() => handleTemporaryChatToggle({ checked: !conversationContext.isTemporaryChat })}
        >
          <div className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}>
            <Icon name='message-bubble-temporary' />
            <span className={styles.menuText}>
              {getLocale(S.AI_CHAT_TEMPORARY_CHAT_LABEL)}
            </span>
            <Toggle
              size='small'
              onChange={handleTemporaryChatToggle}
              checked={conversationContext.isTemporaryChat}
            >
            </Toggle>
          </div>
        </leo-menu-item>
      )}

      {aiChatContext.isStandalone && hasConversationStarted && <>
        <leo-menu-item onClick={() => aiChatContext.setEditingConversationId(conversationContext.conversationUuid!)}>
          <div className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}>
            <Icon name='edit-pencil' />
            <div className={styles.menuText}>
              <div>{getLocale(S.CHAT_UI_MENU_RENAME_CONVERSATION)}</div>
            </div>
          </div>
        </leo-menu-item>
        <leo-menu-item onClick={() => aiChatContext.setDeletingConversationId(conversationContext.conversationUuid!)}>
          <div className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}>
            <Icon name='trash' />
            <div className={styles.menuText}>
              <div>{getLocale(S.CHAT_UI_MENU_DELETE_CONVERSATION)}</div>
            </div>
          </div>
        </leo-menu-item>
        <div className={styles.menuSeparator} />
      </>}

      {!aiChatContext.isPremiumUser && (
        <leo-menu-item onClick={aiChatContext.goPremium}>
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem
            )}
          >
            <Icon name='lock-open' />
            <span className={styles.menuText}>
              {getLocale(S.AI_CHAT_MENU_GO_PREMIUM)}
            </span>
          </div>
        </leo-menu-item>
      )}

      {aiChatContext.isPremiumUser && (
        <leo-menu-item onClick={aiChatContext.managePremium}>
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem
            )}
          >
            <Icon name='lock-open' />
            <span className={styles.menuText}>
              {getLocale(S.CHAT_UI_MENU_MANAGE_SUBSCRIPTION)}
            </span>
          </div>
        </leo-menu-item>
      )}
      {!aiChatContext.isStandalone && aiChatContext.isHistoryFeatureEnabled && (
        <>
          <leo-menu-item
            onClick={() => props.setIsConversationsListOpen?.(true)}
          >
            <div
              className={classnames(
                styles.menuItemWithIcon,
                styles.menuItemMainItem
              )}
            >
              <Icon name='history' />
              <span className={styles.menuText}>{getLocale(S.CHAT_UI_MENU_CONVERSATION_HISTORY)}</span>
            </div>
          </leo-menu-item>
        </>
      )}
      <leo-menu-item onClick={handleSettingsClick}>
        <div
          className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}
        >
          <Icon name='settings' />
          <span className={styles.menuText}>{getLocale(S.CHAT_UI_MENU_SETTINGS)}</span>
        </div>
      </leo-menu-item>
    </ButtonMenu>
  )
}
