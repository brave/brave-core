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
  const content = React.useMemo(
    () => associatedContentInfo.find((c) => c.contentId === tab.contentId),
    [associatedContentInfo, tab],
  )
  return (
    <Checkbox
      className={styles.tabItem}
      checked={!!content}
      onChange={(e) => {
        if (e.checked) {
          aiChat.uiHandler?.associateTab(tab, conversationUuid!)
        } else if (content) {
          aiChat.uiHandler?.disassociateContent(content, conversationUuid!)
        }
      }}
    >
      <div className={styles.itemRow}>
        <span className={styles.title}>{tab.title}</span>
        <span className={styles.subtitle}>{tab.url.url}</span>
      </div>
      <img
        key={tab.contentId}
        className={styles.icon}
        src={`chrome://favicon2/?size=20&pageUrl=${encodeURIComponent(tab.url.url)}`}
      />
    </Checkbox>
  )
}

export default function Attachments() {
  const conversation = useConversation()
  const [search, setSearch] = React.useState('')

  const tabs = React.useMemo(() => {
    const searchLower = search.toLowerCase()
    return conversation.unassociatedTabs.filter((t) =>
      t.title.toLowerCase().includes(searchLower),
    )
  }, [conversation.unassociatedTabs, search])
  return (
    <div className={styles.root}>
      <div className={styles.header}>
        <Flex
          direction='row'
          justify='space-between'
          align='center'
        >
          <h4>{getLocale(S.CHAT_UI_ATTACHMENTS_TABS_TITLE)}</h4>
          <Button
            fab
            kind='plain-faint'
            size='small'
            onClick={() => conversation.setAttachmentsDialog(null)}
          >
            <Icon name='close' />
          </Button>
        </Flex>
        <span className={styles.description}>
          {getLocale(S.CHAT_UI_ATTACHMENTS_TABS_DESCRIPTION)}{' '}
          {getLocale(S.CHAT_UI_ATTACHMENTS_DESCRIPTION_AFTER)}
        </span>
      </div>
      <Input
        className={styles.searchBox}
        placeholder={getLocale(S.CHAT_UI_ATTACHMENTS_TABS_SEARCH_PLACEHOLDER)}
        value={search}
        onInput={(e) => setSearch(e.value)}
      >
        <Icon
          name='search'
          slot='left-icon'
        />
        {search && (
          <Button
            fab
            kind='plain-faint'
            size='small'
            onClick={() => setSearch('')}
            slot='right-icon'
          >
            <Icon name='close' />
          </Button>
        )}
      </Input>
      <div className={styles.tabList}>
        {tabs.map((t) => (
          <TabItem
            key={t.id}
            tab={t}
          />
        ))}
        {tabs.length === 0 && (
          <Flex
            direction='column'
            align='center'
            justify='center'
            className={styles.noResults}
          >
            <span className={styles.noResultsText}>
              {getLocale(S.CHAT_UI_ATTACHMENTS_SEARCH_NO_RESULTS)}
            </span>
            {conversation.unassociatedTabs.length > 0 && (
              <span className={styles.noResultsSuggestion}>
                {getLocale(S.CHAT_UI_ATTACHMENTS_SEARCH_NO_RESULTS_SUGGESTION)}
              </span>
            )}
          </Flex>
        )}
      </div>
    </div>
  )
}
