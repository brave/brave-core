// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import {
  ConversationProvider,
  useConversation,
} from '../../state/conversation_context'
import { useActiveChat } from '../../state/active_chat_context'
import useHasConversationStarted from '../../hooks/useHasConversationStarted'
import InputBox from '../input_box'
import styles from './style.module.scss'

interface Props {
  threadUuid: string
}

// The side-by-side thread panel. Renders the thread's history (including its
// origin entry) in an untrusted iframe and an input box scoped to the thread.
export default function Thread(props: Props) {
  const activeChat = useActiveChat()

  return (
    <ConversationProvider
      {...activeChat}
      threadUuid={props.threadUuid}
    >
      <ThreadContent threadUuid={props.threadUuid} />
    </ConversationProvider>
  )
}

function ThreadContent(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const activeChat = useActiveChat()

  const hasConversationStarted = useHasConversationStarted(
    conversationContext.conversationUuid,
  )

  return (
    <section className={styles.thread}>
      <header className={styles.header}>
        <div className={styles.title}>
          {getLocale(S.CHAT_UI_THREAD_TITLE)}
        </div>
        <div className={styles.closeButton}>
          <Button
            fab
            kind='plain-faint'
            title={getLocale(S.CHAT_UI_CLOSE_BUTTON_LABEL)}
            onClick={() => activeChat.closeThread()}
          >
            <Icon name='close' />
          </Button>
        </div>
      </header>
      <aiChatContext.conversationEntriesComponent
        className={styles.entries}
        threadUuid={props.threadUuid}
      />
      <div className={styles.input}>
        <InputBox
          conversationStarted={hasConversationStarted}
          context={{ ...conversationContext, ...aiChatContext }}
        />
      </div>
    </section>
  )
}
