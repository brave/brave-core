/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import classNames from '$web-common/classnames'
import { useAIChat, useIsSmall } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import ConversationsList from '../conversations_list'
import { NavigationHeader } from '../header'
import Main from '../main'
import styles from './style.module.scss'
import { useActiveChat } from '../../state/active_chat_context'

export default function FullScreen() {
  const aiChatContext = useAIChat()
  const { createNewConversation } = useActiveChat()
  const conversationContext = useConversation()

  const asideRef = React.useRef<HTMLElement | null>(null)
  const isSmall = useIsSmall()

  // Enable CSS transitions after the first paint to avoid animating on mount
  const [isAnimated, setIsAnimated] = React.useState(false)
  React.useEffect(() => {
    const frame = requestAnimationFrame(() => setIsAnimated(true))
    return () => cancelAnimationFrame(frame)
  }, [])

  const canStartNewConversation =
    aiChatContext.hasAcceptedAgreement
    && !!conversationContext.conversationHistory.length

  // When editing a conversation title, ensure the sidebar is open
  React.useEffect(() => {
    if (aiChatContext.editingConversationId && !aiChatContext.showSidebar) {
      aiChatContext.toggleSidebar()
    }
  }, [aiChatContext.editingConversationId])

  // On iOS, close the sidebar when the delete-conversation dialog opens.
  // The sidebar overlaps the dialog and intercepts touch events, requiring
  // a double-tap to reach buttons inside a dialog. Uses toggleSidebar()
  // so showSidebar stays in sync with the visual state.
  // <if expr="is_ios">
  React.useEffect(() => {
    if (aiChatContext.deletingConversationId && aiChatContext.showSidebar) {
      aiChatContext.toggleSidebar()
    }
  }, [aiChatContext.deletingConversationId])
  // </if>

  React.useEffect(() => {
    // We've just changed to small and the sidebar was open, so close it
    if (isSmall && aiChatContext.showSidebar) {
      aiChatContext.toggleSidebar()
    }

    // We've just changed to big and the sidebar was closed, so open it
    if (!isSmall && !aiChatContext.showSidebar) {
      aiChatContext.toggleSidebar()
    }
  }, [isSmall])

  // Add handler for closing the sidebar when clicking outside of it.
  React.useEffect(() => {
    if (!aiChatContext.showSidebar || !isSmall) return
    const handleClick = (e: MouseEvent) => {
      const path = e.composedPath()
      // On iOS, the one-tap fix dispatches synthetic clicks at (0,0).
      // Without this guard, those clicks land outside the sidebar and
      // this handler calls stopPropagation, preventing the click from
      // reaching buttons inside a dialog (e.g. delete confirmation).
      // <if expr="is_ios">
      if (
        path.some((n) => n instanceof Element && n.tagName === 'LEO-DIALOG')
      ) {
        return
      }
      // </if>
      if (!path.includes(asideRef.current!)) {
        aiChatContext.toggleSidebar()
        e.stopPropagation()
      }
    }

    // When the window loses focus (i.e. from iframe click) we should close the side bar.
    const handleBlur = () => {
      aiChatContext.toggleSidebar()
    }

    document.addEventListener('click', handleClick, { capture: true })
    window.addEventListener('blur', handleBlur)
    return () => {
      document.removeEventListener('click', handleClick, { capture: true })
      window.removeEventListener('blur', handleBlur)
    }
  }, [aiChatContext.showSidebar, isSmall])

  return (
    <div
      className={classNames(
        styles.fullscreen,
        aiChatContext.isMobile && isSmall && styles.mobile,
      )}
    >
      <div className={styles.left}>
        {(!aiChatContext.isMobile || !isSmall) && (
          <div className={styles.controls}>
            <Button
              fab
              kind='plain-faint'
              onClick={() => aiChatContext.toggleSidebar()}
            >
              <Icon name='window-tabs-vertical-expanded' />
            </Button>
            {!aiChatContext.showSidebar && canStartNewConversation && (
              <>
                <Button
                  fab
                  kind='plain-faint'
                  onClick={createNewConversation}
                  data-test-id='new-chat-button'
                >
                  <Icon name='edit-box' />
                </Button>
              </>
            )}
          </div>
        )}
        <aside
          ref={asideRef}
          className={classNames(
            styles.aside,
            aiChatContext.showSidebar && styles.open,
            isAnimated && styles.animated,
          )}
        >
          <div className={styles.nav}>
            <NavigationHeader />
            <ConversationsList
              setIsConversationsListOpen={(open) => {
                if (!open && aiChatContext.showSidebar && isSmall) {
                  aiChatContext.toggleSidebar()
                }
              }}
            />
          </div>
        </aside>
      </div>
      <div className={styles.content}>
        <Main />
      </div>
    </div>
  )
}
