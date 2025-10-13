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
import {
  TabData,
  Bookmark,
  HistoryEntry,
} from 'components/ai_chat/resources/common/mojom'
import {
  ConversationContext,
  useConversation,
} from '../../state/conversation_context'
import { getLocale } from '$web-common/locale'
import usePromise from '$web-common/usePromise'

type Attachment = TabData | Bookmark | HistoryEntry

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
        src={`chrome://favicon2/?size=20&pageUrl=${encodeURIComponent(tab.url.url)}&allowGoogleServerFallback=0`}
      />
    </Checkbox>
  )
}

function UrlContentItem({ url, title }: { url: string; title: string }) {
  const aiChat = useAIChat()
  const conversation = useConversation()
  const content = React.useMemo(
    () => conversation.associatedContentInfo.find((c) => c.url.url === url),
    [conversation.associatedContentInfo, url],
  )
  return (
    <Checkbox
      className={styles.tabItem}
      checked={!!content}
      isDisabled={!!content?.conversationTurnUuid}
      onChange={(e) => {
        if (e.checked) {
          aiChat.uiHandler?.associateUrlContent(
            { url },
            title,
            conversation.conversationUuid!,
          )
        } else if (content) {
          aiChat.uiHandler?.disassociateContent(
            content,
            conversation.conversationUuid!,
          )
        }
      }}
    >
      <div className={styles.itemRow}>
        <span className={styles.title}>{title}</span>
        <span className={styles.subtitle}>{url}</span>
      </div>
      <img
        key={url}
        className={styles.icon}
        src={`chrome://favicon2/?size=20&pageUrl=${encodeURIComponent(url)}&allowGoogleServerFallback=0`}
      />
    </Checkbox>
  )
}

type StringKeys = Record<
  NonNullable<ConversationContext['attachmentsDialog']>,
  keyof typeof S
>
const titleKey: StringKeys = {
  tabs: S.CHAT_UI_ATTACHMENTS_TABS_TITLE,
  bookmarks: S.CHAT_UI_ATTACHMENTS_BOOKMARKS_TITLE,
  history: S.CHAT_UI_ATTACHMENTS_HISTORY_TITLE,
}
const descriptionKey: StringKeys = {
  tabs: S.CHAT_UI_ATTACHMENTS_TABS_DESCRIPTION,
  bookmarks: S.CHAT_UI_ATTACHMENTS_BOOKMARKS_DESCRIPTION,
  history: S.CHAT_UI_ATTACHMENTS_HISTORY_DESCRIPTION,
}

const searchPlaceholderKey: StringKeys = {
  tabs: S.CHAT_UI_ATTACHMENTS_TABS_SEARCH_PLACEHOLDER,
  bookmarks: S.CHAT_UI_ATTACHMENTS_BOOKMARKS_SEARCH_PLACEHOLDER,
  history: S.CHAT_UI_ATTACHMENTS_HISTORY_SEARCH_PLACEHOLDER,
}

export function useFilteredItems(search: string) {
  const aiChat = useAIChat()
  const conversation = useConversation()

  const { result: bookmarks = [] } = usePromise(() => aiChat.getBookmarks(), [])
  const { result: history = [] } = usePromise(
    () =>
      conversation.attachmentsDialog === 'history'
        ? // Note: History is only searched for more than 2 characters. This is an upstream limitation.
          aiChat.getHistory(search)
        : Promise.resolve([]),
    [search, conversation.attachmentsDialog],
  )

  return React.useMemo(() => {
    if (!conversation.attachmentsDialog) {
      return []
    }

    const searchLower = search.toLowerCase()
    const filter = (item: { title: string }) =>
      item.title.toLowerCase().includes(searchLower)

    const sources: Record<
      NonNullable<ConversationContext['attachmentsDialog']>,
      Attachment[]
    > = {
      tabs: conversation.unassociatedTabs,
      bookmarks: bookmarks,
      history: history,
    }

    return sources[conversation.attachmentsDialog].filter(filter)
  }, [
    bookmarks,
    history,
    conversation.unassociatedTabs,
    conversation.attachmentsDialog,
    search,
  ])
}

export default function Attachments() {
  const conversation = useConversation()
  const attachmentsDialog = conversation.attachmentsDialog!
  const [search, setSearch] = React.useState('')

  const filteredItems = useFilteredItems(search)
  return (
    <div className={styles.root}>
      <div className={styles.header}>
        <Flex
          direction='row'
          justify='space-between'
          align='center'
        >
          <h4>{getLocale(titleKey[attachmentsDialog])}</h4>
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
          {getLocale(descriptionKey[attachmentsDialog])}{' '}
          {getLocale(S.CHAT_UI_ATTACHMENTS_DESCRIPTION_AFTER)}
        </span>
      </div>
      <Input
        className={styles.searchBox}
        placeholder={getLocale(searchPlaceholderKey[attachmentsDialog])}
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
        {attachmentsDialog === 'tabs'
          && filteredItems.map((t) => (
            <TabItem
              key={t.id}
              tab={t as TabData}
            />
          ))}
        {(attachmentsDialog === 'bookmarks' || attachmentsDialog === 'history')
          && filteredItems.map((b) => (
            <UrlContentItem
              key={b.id}
              url={b.url.url}
              title={b.title}
            />
          ))}
        {filteredItems.length === 0 && (
          <Flex
            direction='column'
            align='center'
            justify='center'
            className={styles.noResults}
          >
            <span className={styles.noResultsText}>
              {getLocale(S.CHAT_UI_ATTACHMENTS_SEARCH_NO_RESULTS)}
            </span>
            {attachmentsDialog === 'tabs'
              && conversation.unassociatedTabs.length > 0 && (
                <span className={styles.noResultsSuggestion}>
                  {getLocale(
                    S.CHAT_UI_ATTACHMENTS_SEARCH_NO_RESULTS_SUGGESTION,
                  )}
                </span>
              )}
          </Flex>
        )}
      </div>
    </div>
  )
}
