// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './inline_tab_search.module.scss'

function TabSourceCard(props: { source: Mojom.TabData }) {
  const context = useUntrustedConversationContext()
  const { source } = props

  const host = (() => {
    try {
      return new URL(source.url.url).hostname
    } catch {
      return source.url.url
    }
  })()

  // Local favicon database only: never fall back to Google's favicon server,
  // which would send the tab's URL off device.
  const faviconSrc =
    `chrome-untrusted://favicon2?size=32&allowGoogleServerFallback=0&pageUrl=`
    + encodeURIComponent(source.url.url)

  const handleClick = () => {
    context.api.uiHandler.switchToTab(source.id)
  }

  return (
    <li>
      <button
        title={source.title || source.url.url}
        onClick={handleClick}
      >
        <img
          className={styles.favicon}
          src={faviconSrc}
          alt=''
        />
        <span className={styles.text}>
          <span className={styles.title}>{source.title || host}</span>
          <span className={styles.host}>{host}</span>
        </span>
      </button>
    </li>
  )
}

// Renders the `::tabSearch[query]` markdown directive: searches the user's
// open tabs on device and shows the matches as cards that switch to the tab.
// The results are never sent to the model.
export default function InlineTabSearch(props: { query: string }) {
  const context = useUntrustedConversationContext()
  const { data: tabs, isLoading } = context.api.useSearchForTabs(props.query)

  if (isLoading) {
    return (
      <p data-testid='inline-tab-search-pending'>
        {formatLocale(S.CHAT_UI_TAB_SEARCH_IN_PROGRESS, { $1: props.query })}
      </p>
    )
  }

  // Null means on-device tab search is unavailable here (non-desktop, or the
  // history embeddings setting is off), which isn't worth telling the user
  // about mid-answer.
  if (!tabs) {
    return null
  }

  if (!tabs.length) {
    return (
      <p data-testid='inline-tab-search-empty'>
        {formatLocale(S.CHAT_UI_TAB_SEARCH_NO_RESULTS, { $1: props.query })}
      </p>
    )
  }

  return (
    <div
      className={styles.inlineTabSearch}
      data-testid='inline-tab-search'
    >
      <ul>
        {tabs.map((source) => (
          <TabSourceCard
            key={source.id}
            source={source}
          />
        ))}
      </ul>
    </div>
  )
}
