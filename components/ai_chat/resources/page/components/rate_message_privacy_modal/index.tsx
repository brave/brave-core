/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import { getLocale, formatLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

const LEARN_MORE_URL = 'https://brave.com/privacy/browser/#your-feedback'

export default function RateMessagePrivacyModal() {
  // Context
  const conversationContext = useConversation()
  const aiChatContext = useAIChat()

  // Methods
  const onClickSend = React.useCallback(() => {
    if (conversationContext.ratingTurnUuid) {
      conversationContext.handleRateMessage(
        conversationContext.ratingTurnUuid.turnUuid,
        conversationContext.ratingTurnUuid.isLiked,
      )
    }
  }, [
    conversationContext.handleRateMessage,
    conversationContext.ratingTurnUuid,
  ])

  const handleLearnMoreClicked = React.useCallback(() => {
    const mojomUrl = new Url()
    mojomUrl.url = LEARN_MORE_URL
    aiChatContext.uiHandler?.openURL(mojomUrl)
  }, [aiChatContext.uiHandler])

  return (
    <Dialog
      isOpen={!!conversationContext.ratingTurnUuid}
      showClose
      onClose={() => conversationContext.handleCloseRateMessagePrivacyModal()}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.dialogTitle}
      >
        {getLocale('rateMessagePrivacyModalTitle')}
      </div>
      <div className={styles.description}>
        <span>
          {formatLocale('rateMessagePrivacyModalDescription', {
            $1: (content) => (
              <a
                // While we preventDefault onClick, we still need to pass
                // the href here so we can show link preview.
                href={LEARN_MORE_URL}
                className={styles.learnMoreLink}
                onClick={(e) => {
                  e.preventDefault()
                  handleLearnMoreClicked()
                }}
              >
                {content}
              </a>
            ),
          })}
        </span>
      </div>
      <div
        slot='actions'
        className={styles.actionsRow}
      >
        <div className={styles.buttonsWrapper}>
          <Button
            kind='plain-faint'
            size='medium'
            onClick={() =>
              conversationContext.handleCloseRateMessagePrivacyModal()
            }
          >
            {getLocale('cancelButtonLabel')}
          </Button>
          <Button
            kind='filled'
            size='medium'
            onClick={onClickSend}
          >
            {getLocale('sendButtonLabel')}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}
