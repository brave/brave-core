/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import useMediaQuery from '$web-common/useMediaQuery'
import { useAIChat } from '../../state/ai_chat_context'
import ConversationsList from '../conversations_list'
import { NavigationHeader } from '../header'
import Main from '../main'
import styles from './style.module.scss'

export default function FullScreen() {
  const aiChatContext = useAIChat()
  const asideAnimationRef = React.useRef<Animation | null>()
  const controllerRef = React.useRef(new AbortController())
  const isSmall = useMediaQuery('(max-width: 1024px)')
  const [isNavigationCollapsed, setIsNavigationCollapsed] = React.useState(isSmall)
  const [isNavigationRendered, setIsNavigationRendered] = React.useState(!isSmall)

  const initAsideAnimation = React.useCallback((node: HTMLElement | null) => {
    if (!node) return
    const open = { width: '340px', opacity: 1 }
    const close = { width: '0px', opacity: 0 }
    const animationOptions: KeyframeAnimationOptions = {
      duration: 200,
      easing: 'ease-out',
      fill: 'forwards'
    }
    asideAnimationRef.current = new Animation(
      new KeyframeEffect(node, [open, close], animationOptions)
    )

    // Make sure we're in the right state for our screen size when
    asideAnimationRef.current.playbackRate = isSmall ? 1 : -1
    asideAnimationRef.current.finish()
  }, [])

  const toggleAside = () => {
    const asideAnimation = asideAnimationRef.current

    if (asideAnimation) {
      if (isNavigationCollapsed) {
        controllerRef.current.abort()
        controllerRef.current = new AbortController()
        asideAnimation.ready.then(() => setIsNavigationRendered(true))
        asideAnimation.playbackRate = -1
      } else {
        // 'finish' triggers in both directions, so we only need this once per close animation
        // user may rapidly toggle the aside, so we need to abort scheduled listener in open animation
        asideAnimation.addEventListener(
          'finish',
          () => setIsNavigationRendered(false),
          { once: true, signal: controllerRef.current.signal }
        )
        asideAnimation.playbackRate = 1
      }

      asideAnimation.play()
      setIsNavigationCollapsed(!isNavigationCollapsed)
    }
  }

  React.useEffect(() => {
    const isOpen = asideAnimationRef.current?.playbackRate === 1
    if (aiChatContext.editingConversationId && isOpen) {
      toggleAside()
    }
  }, [aiChatContext.editingConversationId, isNavigationCollapsed]);

  React.useEffect(() => {
    const isOpen = asideAnimationRef.current?.playbackRate === 1

    // We've just changed to small and the sidebar was open, so close it
    if (isSmall && !isOpen) {
      toggleAside()
    }

    // We've just changed to big and the sidebar was closed, so open it
    if (!isSmall && isOpen) {
      toggleAside()
    }

  }, [isSmall])

  return (
    <div className={styles.fullscreen}>
      <div className={styles.left}>
        <div className={styles.controls}>
          <Button
            fab
            kind='plain-faint'
            onClick={toggleAside}
          >
            <Icon name='window-tabs-vertical-expanded' />
          </Button>
          {!isNavigationRendered && (
            <>
              <Button
                fab
                kind='plain-faint'
                onClick={aiChatContext.onNewConversation}
              >
                <Icon name='edit-box' />
              </Button>
            </>
          )}
        </div>
        <aside
          ref={initAsideAnimation}
          className={styles.aside}
        >
          {isNavigationRendered && (
            <>
              <NavigationHeader />
              <ConversationsList setIsConversationsListOpen={setIsNavigationCollapsed} />
            </>
          )}
        </aside>
      </div>
      <div className={styles.content}>
        <Main />
      </div>
    </div>
  )
}
