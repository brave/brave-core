/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Button from '@brave/leo/react/button'
import Dropdown from '@brave/leo/react/dropdown'
import Icon from '@brave/leo/react/icon'
import * as React from 'react'
import * as nala from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { WebSiteInfoDetail } from '../../api'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import FeatureMenu from '../feature_button_menu'
import SidebarHeader from '../header'
import Main from '../main'
import SidebarNav from '../sidebar_nav'
import styles from './style.module.scss'
import { Url as MojomUrl } from 'gen/url/mojom/url.mojom.m'

const TabEntryListItem = styled.li`
  display: flex;
  align-items: center;
  width: 100%;

  img {
    padding: ${nala.spacing.l};
    border: 1px solid ${nala.color.container.background};
    border-radius: ${nala.radius.s};
  }

  & span {
    flex: 0;
  }
`

function TabImage(props: { url: MojomUrl }) {
  const aiChatContext = useAIChat()
  const [imgUrl, setImgUrl] = React.useState<string>()
  React.useEffect(() => {
    if (!aiChatContext.uiHandler) {
      return
    }
    aiChatContext.uiHandler.getFaviconImageDataForContent(props.url)
    .then(({ faviconImageData }) => {
      if (!faviconImageData) {
        return
      }
      const blob = new Blob([new Uint8Array(faviconImageData)], { type: 'image/*' })
      setImgUrl(URL.createObjectURL(blob))
    })
  }, [props.url.url])

  if (!imgUrl) {
    return null
  }

  return <img src={imgUrl} />
}

function TabEntry({ site }: {
  site: WebSiteInfoDetail
}) {
  const context = useConversation()
  const count = context.associatedContentInfo?.detail?.multipleWebSiteInfo?.sites.length ?? 0

  return <TabEntryListItem>
    <TabImage url={site.url} />
    <span>{site.title}</span>
    {count > 1 && <Button fab kind="plain-faint" onClick={() => context.conversationHandler?.removeTabFromMultiTabContent(site.url)}>
      <Icon name='close' />
    </Button>}
  </TabEntryListItem>
}

function SitePicker() {
  const conversation = useConversation()
  const aiChat = useAIChat()

  const availableSites = React.useMemo(() => {
    const used = new Set(conversation.associatedContentInfo?.detail?.multipleWebSiteInfo?.sites.map(a => a.url.url))
    return aiChat.availableAssociatedContent.filter(w => !used.has(w.url.url))
  }, [aiChat.availableAssociatedContent, conversation.associatedContentInfo])

  return <Dropdown value='' placeholder='Add a tab to the project' onChange={e => conversation.conversationHandler?.addTabToMultiTabContent({ url: e.value ?? '' })}>
    <span slot="value">Add a tab to the conversation</span>
    {availableSites.map((a, i) => <leo-option key={i} value={a.url.url}>
      <div><TabImage url={a.url} /> <span>{a.title}</span></div>
    </leo-option>)}
  </Dropdown>
}

export default function FullScreen() {
  const aiChatContext = useAIChat()
  const chat = useConversation();
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
              <FeatureMenu setIsConversationListOpen={function (value: boolean): unknown {
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
      {!!chat.associatedContentInfo?.detail?.multipleWebSiteInfo && <div className={styles.right}>
        <h3>Tabs used in this conversation</h3>
        <SitePicker />
        <ul>
          {chat.associatedContentInfo.detail.multipleWebSiteInfo.sites.map((t, i) =>
            <TabEntry key={i} site={t} />)}
        </ul>
      </div>}
    </div>
  )
}
