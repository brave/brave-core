// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'
import { showAlert } from '@brave/leo/react/alertCenter'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'
import useHasConversationStarted from '../../hooks/useHasConversationStarted'
import {
  formatConversationForClipboard, //
} from '../../../common/conversation_history_utils'

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
  const hasConversationStarted = useHasConversationStarted(
    conversationContext.conversationUuid,
  )

  const handleTemporaryChatToggle = (detail: { checked: boolean }) => {
    conversationContext.setTemporary(detail.checked)
  }

  const copyEntireConversation = async () => {
    // Fetch the full conversation history directly from the handler
    // to avoid stale closure issues with the context state
    const handler = conversationContext.conversationHandler
    if (!handler) return

    const { conversationHistory } = await handler.getConversationHistory()
    const formattedConversation =
      formatConversationForClipboard(conversationHistory)
    navigator.clipboard.writeText(formattedConversation).then(() => {
      showAlert({
        type: 'info',
        content: getLocale(S.CHAT_UI_CONVERSATION_COPIED),
        actions: [],
      })
    })
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

      {!hasConversationStarted && (
        <leo-menu-item
          data-is-interactive='true'
          onClick={() =>
            handleTemporaryChatToggle({
              checked: !conversationContext.isTemporaryChat,
            })
          }
        >
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem,
            )}
          >
            <Icon name='message-bubble-temporary' />
            <span className={styles.menuText}>
              {getLocale(S.AI_CHAT_TEMPORARY_CHAT_LABEL)}
            </span>
            <Toggle
              size='small'
              onChange={handleTemporaryChatToggle}
              checked={conversationContext.isTemporaryChat}
            ></Toggle>
          </div>
        </leo-menu-item>
      )}

      {hasConversationStarted && (
        <leo-menu-item onClick={() => copyEntireConversation()}>
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem,
            )}
          >
            <Icon name='copy' />
            <span className={styles.menuText}>
              {getLocale(S.CHAT_UI_MENU_COPY_CONVERSATION)}
            </span>
          </div>
        </leo-menu-item>
      )}

      {aiChatContext.isStandalone && hasConversationStarted && (
        <>
          <leo-menu-item
            onClick={() =>
              aiChatContext.setEditingConversationId(
                conversationContext.conversationUuid!,
              )
            }
          >
            <div
              className={classnames(
                styles.menuItemWithIcon,
                styles.menuItemMainItem,
              )}
            >
              <Icon name='edit-pencil' />
              <div className={styles.menuText}>
                <div>{getLocale(S.CHAT_UI_MENU_RENAME_CONVERSATION)}</div>
              </div>
            </div>
          </leo-menu-item>
          <leo-menu-item
            onClick={() =>
              aiChatContext.setDeletingConversationId(
                conversationContext.conversationUuid!,
              )
            }
          >
            <div
              className={classnames(
                styles.menuItemWithIcon,
                styles.menuItemMainItem,
              )}
            >
              <Icon name='trash' />
              <div className={styles.menuText}>
                <div>{getLocale(S.CHAT_UI_MENU_DELETE_CONVERSATION)}</div>
              </div>
            </div>
          </leo-menu-item>
        </>
      )}
      <div className={styles.menuSeparator} />
      {!aiChatContext.isStandalone && aiChatContext.isHistoryFeatureEnabled && (
        <>
          <leo-menu-item
            onClick={() => props.setIsConversationsListOpen?.(true)}
          >
            <div
              className={classnames(
                styles.menuItemWithIcon,
                styles.menuItemMainItem,
              )}
            >
              <Icon name='history' />
              <span className={styles.menuText}>
                {getLocale(S.CHAT_UI_MENU_CONVERSATION_HISTORY)}
              </span>
            </div>
          </leo-menu-item>
        </>
      )}
      {!aiChatContext.isMobile && (
        <leo-menu-item
          onClick={() => aiChatContext.uiHandler?.openMemorySettings()}
        >
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem,
            )}
          >
            <Icon name='database' />
            <span className={styles.menuText}>
              {getLocale(S.CHAT_UI_MENU_MANAGE_MEMORIES)}
            </span>
          </div>
        </leo-menu-item>
      )}
      {!aiChatContext.isPremiumUser && (
        <leo-menu-item onClick={aiChatContext.goPremium}>
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem,
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
              styles.menuItemMainItem,
            )}
          >
            <Icon name='lock-open' />
            <span className={styles.menuText}>
              {getLocale(S.CHAT_UI_MENU_MANAGE_SUBSCRIPTION)}
            </span>
          </div>
        </leo-menu-item>
      )}

      <leo-menu-item onClick={handleSettingsClick}>
        <div
          className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem,
          )}
        >
          <Icon name='settings' />
          <span className={styles.menuText}>
            {getLocale(S.CHAT_UI_MENU_SETTINGS)}
          </span>
        </div>
      </leo-menu-item>
    </ButtonMenu>
  )
}
