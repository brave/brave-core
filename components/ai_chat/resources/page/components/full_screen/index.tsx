/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import styles from './style.module.scss'
import Main from '../main'
import SidebarHeader from '../header'
import SidebarNav from '../sidebar_nav'
import FeatureMenu from '../feature_button_menu'
import { useAIChat } from '../../state/ai_chat_context'

export default function FullScreen() {
  const aiChatContext = useAIChat()
  const asideAnimationRef = React.useRef<Animation | null>()
  const controllerRef = React.useRef(new AbortController())
  const [isOpen, setIsOpen] = React.useState(false)
  const [shouldRender, setShouldRender] = React.useState(true)

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
  }, [])

  const toggleAside = () => {
    const asideAnimation = asideAnimationRef.current

    if (asideAnimation) {
      if (isOpen) {
        controllerRef.current.abort()
        controllerRef.current = new AbortController()
        asideAnimation.ready.then(() => setShouldRender(true))
        asideAnimation.playbackRate = -1
      } else {
        // 'finish' triggers in both directions, so we only need this once per close animation
        // user may rapidly toggle the aside, so we need to abort scheduled listener in open animation
        asideAnimation.addEventListener(
          'finish',
          () => setShouldRender(false),
          { once: true, signal: controllerRef.current.signal }
        )
        asideAnimation.playbackRate = 1
      }

      asideAnimation.play()
      setIsOpen(!isOpen)
    }
  }

  const handleEraseClick = () => {
    aiChatContext.onNewConversation()
  }

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
          {!shouldRender && (
            <>
              <Button
                fab
                kind='plain-faint'
                onClick={handleEraseClick}
              >
                <Icon name='erase' />
              </Button>
              <FeatureMenu  setIsConversationListOpen={function (value: boolean): unknown {
                throw new Error('Function not implemented.')
              }} />
            </>
          )}
        </div>
        <aside
          ref={initAsideAnimation}
          className={styles.aside}
        >
          {shouldRender && (
            <>
              <SidebarHeader />
              <SidebarNav setIsConversationListOpen={setIsOpen} />
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
