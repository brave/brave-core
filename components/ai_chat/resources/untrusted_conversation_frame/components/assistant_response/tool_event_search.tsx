// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { formatLocale, getLocale } from '$web-common/locale'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event_search.module.scss'
import '../../../common/strings'
import {
  getBraveSearchUrlForQuery,
  LEO_BRAVE_SEARCH_SUPPORT_URL,
} from '../../../common/constants'

function SearchSummary(props: { searchQueries: string[] }) {
  const context = useUntrustedConversationContext()

  const handleLearnMoreClick = (e: React.MouseEvent<HTMLAnchorElement>) => {
    e.preventDefault()
    context.uiHandler?.openLearnMoreAboutBraveSearchWithLeo()
  }

  const message = formatLocale(S.CHAT_UI_SEARCH_QUERIES, {
    $1: props.searchQueries.map((query, i, a) => (
      <React.Fragment key={i}>
        <a
          className={styles.searchQueryLink}
          href={getBraveSearchUrlForQuery(query)}
          target='_blank'
          rel='noopener noreferrer'
        >
          {`"${query}"`}
        </a>
        {i < a.length - 1 ? ', ' : null}
      </React.Fragment>
    )),
  })

  return (
    <div className={styles.searchSummary}>
      <span data-testid='search-summary'>
        {message}{' '}
        <a
          className={styles.searchLearnMoreLink}
          href={LEO_BRAVE_SEARCH_SUPPORT_URL}
          onClick={handleLearnMoreClick}
        >
          {getLocale(S.CHAT_UI_LEARN_MORE)}
        </a>
      </span>
    </div>
  )
}

const ToolEventSearch: ToolComponent = (props) => {
  const queries: string[] = [
    ...(props.toolInput?.query ?? []),
    ...(props.toolUseEvent.output?.[0]?.webSourcesContentBlock?.queries ?? []),
  ]

  const content: ToolUseContent = {
    toolLabel: queries.length ? null : props.content.toolLabel,
    expandedContent: !queries.length ? null : (
      <SearchSummary searchQueries={queries} />
    ),
  }

  return props.children(content)
}

export default ToolEventSearch
