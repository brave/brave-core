// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { TabData } from "components/ai_chat/resources/common/mojom";
import FilterMenu from "./filter_menu";
import styles from "./style.module.scss";
import * as React from "react";
import { extractQuery, matches } from "./query";
import { useAIChat } from "../../state/ai_chat_context";
import { useConversation } from "../../state/conversation_context";
import { getLocale } from "$web-common/locale";

function matchesQuery(query: string, entry: TabData) {
  return matches(query, entry.title) || matches(query, entry.url.url)
}

export default function TabsMenu() {
  const aiChat = useAIChat()
  const conversation = useConversation()
  
  const query = extractQuery(conversation.inputText, {
    onlyAtStart: true,
    triggerCharacter: '@',
  })

  const isOpen = React.useMemo(() => query !== null, [query])

  const setIsOpen = React.useCallback((isOpen: boolean) => {
    if (!isOpen) {
      conversation.setInputText('')
      document.querySelector('textarea')?.focus()
    }
  }, [conversation.setInputText])

  const selectTab = React.useCallback((tab: TabData) => {
    setIsOpen(false)
    aiChat.uiHandler?.associateTab(tab, conversation.conversationUuid!)
    document.querySelector('textarea')?.focus()
  }, [aiChat, conversation.conversationUuid, setIsOpen])

  const unselectedTabs = React.useMemo(() => aiChat.tabs
    .filter(t => !conversation.associatedContentInfo.some(c => c.contentId === t.contentId)),
    [aiChat.tabs, conversation.associatedContentInfo])

  return <FilterMenu
    categories={[{
      category: getLocale('tabsMenuTitle'),
      entries: unselectedTabs
    }]}
    isOpen={isOpen}
    setIsOpen={setIsOpen}
    query={query}
    matchesQuery={matchesQuery}
  >
    {(item) => <leo-menu-item
        class={styles.tabMenuItem}
        key={item.contentId}
        onClick={() => selectTab(item)}
      >
        <img src={`//favicon2?pageUrl=${encodeURIComponent(item.url.url)}`} />
        {item.title}
      </leo-menu-item>}
  </FilterMenu>
}
