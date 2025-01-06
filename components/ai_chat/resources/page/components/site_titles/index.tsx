/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'
import { useAIChat } from '../../state/ai_chat_context'
import { SiteInfoDetail } from 'components/ai_chat/resources/common/mojom'
import Icon from '@brave/leo/react/icon'
import Flex from '$web-common/Flex'
import Button from '@brave/leo/react/button'

interface SiteTitleProps {
  size: 'default' | 'small'
  detail: SiteInfoDetail
}

function SiteTitle(props: SiteTitleProps) {
  const conversation = useConversation()
  const aiChat = useAIChat()
  const tab = aiChat.tabs.find(t => t.url.url === props.detail.url.url)

  // We can remove a tab from the context if:
  // 1. The conversation has not started (i.e its not in the visible conversation list).
  // 2. We're in standalone mode.
  const removable = tab
    && !aiChat.visibleConversations.some(c => c.uuid === conversation.conversationUuid)
    && aiChat.isStandalone

  return (
    <div
      className={classnames({
        [styles.box]: true,
        [styles.boxSm]: props.size === 'small'
      })}
    >
      <div
        className={classnames({
          [styles.favIconContainer]: true,
          [styles.favIconContainerSm]: props.size === 'small'
        })}
      >
        {conversation.faviconUrl && <img src={conversation.faviconUrl} />}
      </div>
      <div
        className={classnames({
          [styles.titleContainer]: true,
          [styles.titleContainerSm]: props.size === 'small'
        })}
      >
        <p
          className={styles.title}
          title={props.detail.title}
        >
          {props.detail.title}
        </p>
      </div>
      {removable && <Button fab kind='plain-faint' onClick={() => conversation.conversationHandler?.removeAssociatedTab(tab)}>
        <Icon name='trash' />
      </Button>}
    </div>
  )
}

function SiteTitles(props: { size: 'default' | 'small' }) {
  const conversation = useConversation()

  return (
    <Flex direction='column' gap={8}>
      {conversation.associatedContentInfo?.details?.map((tab) => <SiteTitle key={tab.url.url} size={props.size} detail={tab} />)}
    </Flex>
  )
}

export default SiteTitles
