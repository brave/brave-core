// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styles from './style.module.scss'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'
import Flex from '$web-common/Flex'
import { useAIChat } from '../../state/ai_chat_context'
import { TabData } from 'components/ai_chat/resources/common/mojom'
import { useConversation } from '../../state/conversation_context'
import { getLocale } from '$web-common/locale'

function TabItem({ tab }: { tab: TabData }) {
  const aiChat = useAIChat()
  const { conversationUuid, associatedContentInfo } = useConversation()
  return (
    <Checkbox
      className={styles.tabItem}
      checked={associatedContentInfo.some((c) => c.contentId === tab.contentId)}
      onChange={(e) => {
        if (e.checked) {
          aiChat.uiHandler?.associateTab(tab, conversationUuid!)
        } else {
          aiChat.uiHandler?.disassociateTab(tab, conversationUuid!)
        }
      }}
    >
      <span className={styles.title}>{tab.title}</span>
      <img
        key={tab.contentId}
        className={styles.icon}
        src={`chrome://favicon2/?size=20&pageUrl=${encodeURIComponent(tab.url.url)}`}
      />
    </Checkbox>
  )
}

export default function Attachments() {
  const aiChat = useAIChat()
  const conversation = useConversation()
  const [search, setSearch] = React.useState('')

  const tabs = aiChat.tabs.filter((t) =>
    t.title.toLowerCase().includes(search.toLowerCase()),
  )
  return (
    <div className={styles.root}>
      <div className={styles.header}>
        <Flex
          direction='row'
          justify='space-between'
          align='center'
        >
          <h4>{getLocale('attachmentsTitle')}</h4>
          <Button
            fab
            kind='plain-faint'
            size='small'
            onClick={() => conversation.setShowAttachments(false)}
          >
            <Icon name='close' />
          </Button>
        </Flex>
        <span className={styles.description}>
          {getLocale('attachmentsDescription')}
        </span>
      </div>
      <div className={styles.tabSearchContainer}>
        <Flex
          direction='row'
          justify='space-between'
          align='center'
        >
          <h5>{getLocale('attachmentsBrowserTabsTitle')}</h5>
        </Flex>
        <Input
          placeholder={getLocale('searchTabsPlaceholder')}
          value={search}
          onInput={(e) => setSearch(e.value)}
        >
          <Icon
            name='search'
            slot='icon-after'
          />
        </Input>
        <div className={styles.tabList}>
          {tabs.map((t) => (
            <TabItem
              key={t.id}
              tab={t}
            />
          ))}
        </div>
      </div>
    </div>
  )
}
