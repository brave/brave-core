/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import useMediaQuery from '$web-common/useMediaQuery'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import ConversationsList from '../conversations_list'
import { NavigationHeader } from '../header'
import Main from '../main'
import styles from './style.module.scss'
import { useActiveChat } from '../../state/active_chat_context'
import { flushSync } from 'react-dom'

export default function FullScreen() {
  const aiChatContext = useAIChat()
  const { createNewConversation } = useActiveChat()
  const conversationContext = useConversation()

  const isSmall = useMediaQuery('(max-width: 1024px)')

  const [showSidebar, setShowSidebar] = React.useState(!isSmall)
  const toggleSidebar = () => {
    (document as any).startViewTransition({
      update: () => {
        flushSync(() => {
          setShowSidebar(s => !s)
        })
      },
      types: [showSidebar ? 'close' : 'open']
    })
  }

  const canStartNewConversation = aiChatContext.hasAcceptedAgreement &&
    !!conversationContext.conversationHistory.length

  React.useEffect(() => {
    // We've just changed to small and the sidebar was open, so close it
    if (isSmall && showSidebar) {
      toggleSidebar()
    }

    // We've just changed to big and the sidebar was closed, so open it
    if (!isSmall && !showSidebar) {
      toggleSidebar()
    }

  }, [isSmall])

  return (
    <div className={styles.fullscreen}>
      <div className={styles.left}>
        <div className={styles.controls}>
          <Button
            fab
            kind='plain-faint'
            onClick={toggleSidebar}
          >
            <Icon name={showSidebar ? 'sidenav-collapse' : 'sidenav-expand'} />
          </Button>
          {!showSidebar && canStartNewConversation && (
            <>
              <Button
                fab
                kind='plain-faint'
                onClick={createNewConversation}
              >
                <Icon name='edit-box' />
              </Button>
            </>
          )}
        </div>
        <aside
          className={styles.aside}
        >
          {showSidebar && (
            <div className={styles.nav}>
              <NavigationHeader />
              <ConversationsList setIsConversationsListOpen={toggleSidebar} />
            </div>
          )}
        </aside>
      </div>
      <div className={styles.content}>
        <Main />
      </div>
    </div>
  )
}
