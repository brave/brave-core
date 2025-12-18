/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

export default function OpenExternalLinkModal() {
  // Context
  const conversationContext = useConversation()
  const aiChatContext = useAIChat()

  // State
  const [ignoreChecked, setIgnoreChecked] = React.useState(false)

  // Methods
  const onOpenClicked = React.useCallback(() => {
    if (ignoreChecked) {
      conversationContext.setIgnoreExternalLinkWarning()
    }
    if (conversationContext.generatedUrlToBeOpened) {
      aiChatContext.api.actions.uiHandler.openURL(
        conversationContext.generatedUrlToBeOpened,
      )
    }
    conversationContext.setGeneratedUrlToBeOpened(undefined)
  }, [
    ignoreChecked,
    conversationContext.generatedUrlToBeOpened,
    conversationContext.setIgnoreExternalLinkWarning,
    conversationContext.setGeneratedUrlToBeOpened,
    aiChatContext.api.actions.uiHandler.openURL,
  ])

  return (
    <Dialog
      isOpen={!!conversationContext.generatedUrlToBeOpened}
      showClose
      onClose={() => conversationContext.setGeneratedUrlToBeOpened(undefined)}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.dialogTitle}
      >
        {getLocale(S.CHAT_UI_OPEN_EXTERNAL_LINK)}
      </div>
      <div className={styles.description}>
        {getLocale(S.CHAT_UI_OPEN_EXTERNAL_LINK_INFO)}
        <Checkbox
          checked={ignoreChecked}
          onChange={({ checked }) => setIgnoreChecked(checked)}
        >
          <span>{getLocale(S.CHAT_UI_OPEN_EXTERNAL_LINK_CHECKBOX_LABEL)}</span>
        </Checkbox>
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
              conversationContext.setGeneratedUrlToBeOpened(undefined)
            }
          >
            {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
          </Button>
          <Button
            kind='filled'
            size='medium'
            onClick={onOpenClicked}
          >
            {getLocale(S.CHAT_UI_OPEN_LABEL)}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}
