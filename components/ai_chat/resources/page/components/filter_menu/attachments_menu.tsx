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

  const [isOpen, setIsOpen] = React.useState(false)
  React.useEffect(() => {
    if (query !== null) {
      setIsOpen(true)
    }
  }, [query !== null])

  const handleClose = React.useCallback((isOpen: boolean) => {
    if (isOpen) return

    const editor = document.querySelector<HTMLElement>('[data-editor]')
    if (!editor) return

    // If we have a trigger character, but the menu is closed make sure we remove it.
    makeEdit(editor)
      .selectRangeToTriggerChar('@')
      .ifHasSelection()
      ?.replaceSelectedRange('')

    editor?.focus()
  }, [])

  const selectAttachment = React.useCallback(
    (attachment: Attachment) => {
      const alreadyAttached = conversation.associatedContentInfo.find((c) =>
        'contentId' in attachment
          ? c.contentId === attachment.contentId
          : c.url.url === attachment.url.url,
      )

      if (alreadyAttached) {
        // Re-mentioning content that's already attached re-attaches its tools
        // (the browser only attaches them when the content exposes any).
        conversation.setToolsAttached(alreadyAttached, true)
      } else if ('contentId' in attachment) {
        aiChat.api.uiHandler.associateTab(
          attachment,
          conversation.conversationUuid!,
        )
      } else {
        aiChat.api.uiHandler.associateUrlContent(
          attachment.url,
          attachment.title,
          conversation.conversationUuid!,
        )
      }

      const editor = document.querySelector<HTMLElement>('[data-editor]')
      if (editor) {
        makeEdit(editor)
          .selectRangeToTriggerChar('@')
          .ifHasSelection()
          ?.replaceSelectedRange({
            id: attachment.id.toString(),
            text: attachment.title,
            url: attachment.url.url,
            type: 'attachment',
          })
      }

      setIsOpen(false)
    },
    [
      aiChat,
      conversation.conversationUuid,
      conversation.associatedContentInfo,
      conversation.setToolsAttached,
      setIsOpen,
    ],
  )

  // Show all open tabs, including ones already associated with the
  // conversation. Re-mentioning an attached tab re-runs tool detection on the
  // browser side, re-attaching its tools when it exposes any. Bookmarks and
  // history are still de-duplicated below because re-attaching those creates
  // duplicate content instead of reusing a live tab.
  //
  // This is an owned copy of the tabs list: `distinctEntries` appends bookmarks
  // and history into it, so it must not be the shared `aiChat.tabs` array.
  const tabs = React.useMemo(() => [...aiChat.tabs], [aiChat.tabs])

  const bookmarks = aiChat.api.useGetBookmarksData()

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

  const history = aiChat.api.useGetHistoryData(query ?? '', null)

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
    const result: Attachment[] = tabs
    const seen = new Set<string>(tabs.map((t) => t.url.url))
    for (const item of unselectedBookmarks.concat(unselectedHistory)) {
      if (seen.has(item.url.url)) continue

      result.push(item)
      seen.add(item.url.url)
    }
    return result
  }, [tabs, unselectedBookmarks, unselectedHistory])

  return (
    <FilterMenu
      categories={[
        {
          category: '',
          entries: distinctEntries,
        },
      ]}
      isOpen={isOpen}
      setIsOpen={handleClose}
      query={query}
      matchesQuery={matchesQuery}
      header={
        <div className={styles.tabsMenuHeader}>
          {getLocale(S.CHAT_UI_ATTACHMENTS_MENU_TITLE)}
        </div>
      }
      onResultsChanged={(results) => {
        const hasMatches = results.some((result) => result.entries.length > 0)
        setIsOpen(query !== null && hasMatches)
      }}
      isFullWidth
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
