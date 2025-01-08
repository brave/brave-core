/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './style.module.scss'

export default function DeleteConversationModal() {
  // Context
  const aiChatContext = useAIChat()

  // Computed
  const title = aiChatContext.visibleConversations.find(
        (conversation) =>
          conversation.uuid === aiChatContext.deletingConversationId
      )?.title || getLocale('conversationListUntitled')

  return (
    <Dialog
      isOpen={!!aiChatContext.deletingConversationId}
      showClose
      onClose={() => aiChatContext.setDeletingConversationId(null)}
      className={styles.deleteConversationDialog}
    >
      <div
        slot='title'
        className={styles.deleteConversationDialogTitle}
      >
        {getLocale('menuDeleteConversation')}
      </div>
      <div className={styles.deleteConversationBody}>
        <div className={styles.conversationNameWrapper}>{title}</div>
        {getLocale('deleteConversationWarning')}
      </div>
      <div
        slot='actions'
        className={styles.deleteConversationActionsRow}
      >
        <div className={styles.buttonsWrapper}>
          <Button
            kind='plain-faint'
            size='medium'
            onClick={() => aiChatContext.setDeletingConversationId(null)}
          >
            {getLocale('cancelButtonLabel')}
          </Button>
          <Button
            kind='filled'
            size='medium'
            onClick={() => {
              if (aiChatContext.deletingConversationId) {
                aiChatContext.service?.deleteConversation(
                  aiChatContext.deletingConversationId
                )
                aiChatContext.setDeletingConversationId(null)
              }
            }}
          >
            {getLocale('deleteButtonLabel')}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}
