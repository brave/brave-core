// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Bookmark, TabData } from '../../../common/mojom'
import FilterMenu, { MatchedText } from './filter_menu'
import styles from './style.module.scss'
import * as React from 'react'
import { useExtractedQuery, matches } from './query'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { getLocale } from '$web-common/locale'
import usePromise from '$web-common/usePromise'
import { FuzzyFinder } from './fuzzy_finder'
import { makeEdit, stringifyContent } from '../input_box/editable_content'
import Icon from '@brave/leo/react/icon'

type Attachment = TabData | Bookmark

function matchesQuery(query: FuzzyFinder, entry: Attachment) {
  return matches(query, entry.title)
}

const getType = (entry: Attachment) => {
  if ('contentId' in entry) {
    return 'tab'
  }
  if (entry instanceof Bookmark) {
    return 'bookmark'
  }
  return 'history'
}

const icon: Record<ReturnType<typeof getType>, string> = {
  tab: 'window-tab',
  history: 'history',
  bookmark: 'browser-bookmark-normal',
}

export default function TabsMenu() {
  const aiChat = useAIChat()
  const conversation = useConversation()

  const query = useExtractedQuery(stringifyContent(conversation.inputText), {
    onlyAtStart: false,
    triggerCharacter: '@',
  })

  const isOpen = React.useMemo(() => query !== null, [query])

  const setIsOpen = React.useCallback(
    (isOpen: boolean) => {
      if (!isOpen) {
        const editor = document.querySelector<HTMLElement>('[data-editor]')
        if (!editor) return

        makeEdit(editor).selectRangeToTriggerChar('@').replaceSelectedRange('')

        editor?.focus()
      }
    },
    [conversation.setInputText],
  )

  const selectAttachment = React.useCallback(
    (attachment: Attachment) => {
      setIsOpen(false)
      if ('contentId' in attachment) {
        aiChat.uiHandler?.associateTab(
          attachment,
          conversation.conversationUuid!,
        )
      } else {
        aiChat.uiHandler?.associateUrlContent(
          attachment.url,
          attachment.title,
          conversation.conversationUuid!,
        )
      }
      document.querySelector('textarea')?.focus()
    },
    [aiChat, conversation.conversationUuid, setIsOpen],
  )

  // Filter out content that is already associated with the conversation.
  const unselectedTabs = React.useMemo(
    () =>
      aiChat.tabs.filter(
        (t) =>
          !conversation.associatedContentInfo.some(
            (c) => c.contentId === t.contentId,
          ),
      ),
    [aiChat.tabs, conversation.associatedContentInfo],
  )

  const { result: bookmarks = [] } = usePromise(aiChat.getBookmarks, [])

  // Filter out content that is already associated with the conversation.
  const unselectedBookmarks = React.useMemo(
    () =>
      bookmarks.filter(
        (b) =>
          !conversation.associatedContentInfo.some(
            (c) => c.url.url === b.url.url,
          ),
      ),
    [bookmarks, conversation.associatedContentInfo],
  )

  const { result: history = [] } = usePromise(
    () => aiChat.getHistory(query ?? ''),
    [query],
  )

  // Filter out content that is already associated with the conversation.
  const unselectedHistory = React.useMemo(
    () =>
      history.filter(
        (h) =>
          !conversation.associatedContentInfo.some(
            (c) => c.url.url === h.url.url,
          ),
      ),
    [history, conversation.associatedContentInfo],
  )

  const distinctEntries = React.useMemo(() => {
    // Note: We include all tabs, even if they're duplicated.
    const result: Attachment[] = unselectedTabs
    const seen = new Set<string>(unselectedTabs.map((t) => t.url.url))
    for (const item of unselectedBookmarks.concat(unselectedHistory)) {
      if (seen.has(item.url.url)) continue

      result.push(item)
      seen.add(item.url.url)
    }
    return result
  }, [unselectedTabs, unselectedBookmarks, unselectedHistory])

  return (
    <FilterMenu
      categories={[
        {
          category: '',
          entries: distinctEntries,
        },
      ]}
      isOpen={isOpen}
      setIsOpen={setIsOpen}
      query={query}
      matchesQuery={matchesQuery}
      header={
        <div className={styles.tabsMenuHeader}>
          {getLocale(S.CHAT_UI_ATTACHMENTS_MENU_TITLE)}
        </div>
      }
      noMatchesMessage={
        <div className={styles.tabNoMatches}>
          {getLocale(S.CHAT_UI_ATTACHMENTS_MENU_NO_MATCHING_ATTACHMENTS)}
        </div>
      }
    >
      {(item, category, match) => (
        <leo-menu-item
          class={styles.tabMenuItem}
          key={item.id.toString()}
          onClick={() => selectAttachment(item)}
        >
          <img src={`//favicon2?pageUrl=${encodeURIComponent(item.url.url)}`} />
          <div className={styles.tabItemInfo}>
            <MatchedText
              text={item.title}
              match={match}
            />
          </div>
          <Icon name={icon[getType(item)]} />
        </leo-menu-item>
      )}
    </FilterMenu>
  )
}
