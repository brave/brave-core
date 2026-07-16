/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import { serializeConversationForSharing } from '../../../common/conversation_serialization'
import { encryptForSharing } from '../../../common/conversation_share_encryption'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

interface Props {
  isOpen: boolean
  onClose: () => void
}

export default function ShareConversationModal(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const [isGenerating, setIsGenerating] = React.useState(false)
  const [isCopied, setIsCopied] = React.useState(false)

  const conversationUuid =
    conversationContext.api.useGetStateData().conversationUuid
  const allConversations = aiChatContext.api.useGetConversationsData()

  const conversationTitle = React.useMemo(() => {
    const conversation = allConversations.find(
      (c) => c.uuid === conversationUuid,
    )
    return conversation?.title || ''
  }, [allConversations, conversationUuid])

  // Reset back to the initial "Generate link" state each time the dialog is
  // (re)opened, so a subsequent share doesn't start from the "copied" state.
  React.useEffect(() => {
    if (props.isOpen) {
      setIsGenerating(false)
      setIsCopied(false)
    }
  }, [props.isOpen])

  const handleGenerateLink = async () => {
    // Do not generate multiple times
    if (isGenerating || isCopied) {
      return
    }

    setIsGenerating(true)
    try {
      // Encrypt the conversation in the client so the sharing server only ever
      // sees ciphertext. The decryption key stays on the client and is appended
      // to the returned viewer URL as a fragment, so the shared link is only
      // usable by whoever the user shares it with.
      const json = serializeConversationForSharing(
        conversationContext.api.getConversationHistory.current(),
      )
      const { ciphertext, keyFragment } = await encryptForSharing(json)
      // The browser process combines the key fragment with the server's viewer
      // URL and copies the resulting shareable link to the clipboard, marked as
      // confidential (excluded from clipboard history and other
      // data-leak-prevention surfaces) - something the renderer cannot do. The
      // key fragment is not sent to the sharing server.
      const { sharedConversationUrl } =
        await aiChatContext.api.service.shareConversation(
          ciphertext,
          keyFragment,
          /*copyToClipboard=*/ true,
        )
      // A null result means sharing failed and nothing was copied, so leave the
      // button in its initial state to allow retrying.
      if (sharedConversationUrl) {
        setIsCopied(true)
      }
    } finally {
      setIsGenerating(false)
    }
  }

  return (
    <Dialog
      isOpen={props.isOpen}
      showClose
      onClose={props.onClose}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.title}
      >
        {getLocale(S.CHAT_UI_SHARE_CONVERSATION_LABEL)}
      </div>
      <div className={styles.body}>
        {conversationTitle && (
          <div className={styles.conversationName}>{conversationTitle}</div>
        )}
        <div className={styles.description}>
          {getLocale(S.CHAT_UI_SHARE_CONVERSATION_DIALOG_DESCRIPTION)}
        </div>
      </div>
      <div slot='actions'>
        <Button
          kind='filled'
          size='medium'
          isLoading={isGenerating}
          className={isCopied ? styles.buttonCopied : styles.button}
          onClick={handleGenerateLink}
        >
          <Icon
            slot='icon-before'
            name={isCopied ? 'check-circle-outline' : 'link-normal'}
          />
          {isCopied
            ? getLocale(
                S.CHAT_UI_SHARE_CONVERSATION_DIALOG_LINK_COPIED_BUTTON_LABEL,
              )
            : getLocale(
                S.CHAT_UI_SHARE_CONVERSATION_DIALOG_GENERATE_LINK_BUTTON_LABEL,
              )}
        </Button>
      </div>
    </Dialog>
  )
}
