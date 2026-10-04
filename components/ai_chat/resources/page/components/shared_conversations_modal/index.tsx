/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import { showAlert } from '@brave/leo/react/alertCenter'
import { getLocale } from '$web-common/locale'
import { mojoTimeToJSDate } from '$web-common/mojomUtils'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './style.module.scss'

interface Props {
  onClose: () => void
}

const dateTimeFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric',
  year: 'numeric',
  hour: 'numeric',
  minute: 'numeric',
})

const copyLinkLabel = getLocale(
  S.CHAT_UI_SHARED_CONVERSATIONS_COPY_LINK_BUTTON_LABEL,
)
const deleteLabel = getLocale(
  S.CHAT_UI_SHARED_CONVERSATIONS_DELETE_BUTTON_LABEL,
)

export default function SharedConversationsModal(props: Props) {
  const aiChatContext = useAIChat()

  const { getConversationSharesData: shares, isPlaceholderData: isLoading } =
    aiChatContext.api.useGetConversationShares()
  const {
    mutateAsync: deleteConversationShare,
    isPending: isDeleting,
    variables: deletingArgs,
  } = aiChatContext.api.useDeleteConversationShare()

  const handleCopyLink = (shareId: string) => {
    // The link is copied by the browser process: it holds the conversation's
    // decryption key, and only the browser can mark the clipboard entry as
    // confidential.
    aiChatContext.api.service.copyConversationShareLink(shareId)
    showAlert({
      type: 'info',
      content: getLocale(
        S.CHAT_UI_SHARE_CONVERSATION_DIALOG_LINK_COPIED_BUTTON_LABEL,
      ),
      actions: [],
    })
  }

  const handleDelete = async (shareId: string) => {
    // The record is only removed once the server confirms the deletion, so a
    // failure leaves the entry in place for the user to retry.
    const success = await deleteConversationShare([shareId])
    if (!success) {
      showAlert({
        type: 'error',
        content: getLocale(S.CHAT_UI_SHARED_CONVERSATIONS_DELETE_ERROR),
        actions: [],
      })
    }
  }

  return (
    <Dialog
      isOpen
      showClose
      onClose={props.onClose}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.title}
      >
        {getLocale(S.CHAT_UI_MENU_MANAGE_SHARED_CONVERSATIONS)}
      </div>
      <div className={styles.body}>
        <div className={styles.description}>
          {getLocale(S.CHAT_UI_SHARED_CONVERSATIONS_DIALOG_DESCRIPTION)}
        </div>
        {isLoading ? (
          <div className={styles.placeholder}>
            <ProgressRing />
          </div>
        ) : shares.length === 0 ? (
          <div className={styles.placeholder}>
            {getLocale(S.CHAT_UI_SHARED_CONVERSATIONS_DIALOG_EMPTY)}
          </div>
        ) : (
          <ul className={styles.list}>
            {shares.map((share) => (
              <li
                key={share.shareId}
                className={styles.share}
              >
                <div className={styles.shareDetails}>
                  <div className={styles.shareTitle}>
                    {share.conversationTitle
                      || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)}
                  </div>
                  <div className={styles.shareMeta}>
                    <span>
                      {dateTimeFormatter.format(
                        mojoTimeToJSDate(share.createdTime),
                      )}
                    </span>
                    <span className={styles.shareId}>{share.shareId}</span>
                  </div>
                </div>
                <div className={styles.shareActions}>
                  <Button
                    fab
                    kind='plain-faint'
                    size='small'
                    title={copyLinkLabel}
                    aria-label={copyLinkLabel}
                    onClick={() => handleCopyLink(share.shareId)}
                  >
                    <Icon name='copy' />
                  </Button>
                  <Button
                    fab
                    kind='plain-faint'
                    size='small'
                    title={deleteLabel}
                    aria-label={deleteLabel}
                    isLoading={
                      isDeleting && deletingArgs?.[0] === share.shareId
                    }
                    onClick={() => handleDelete(share.shareId)}
                  >
                    <Icon name='trash' />
                  </Button>
                </div>
              </li>
            ))}
          </ul>
        )}
      </div>
    </Dialog>
  )
}
