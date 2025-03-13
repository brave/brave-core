// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import MarkdownRenderer from '../markdown_renderer'
import WebSourcesEvent from './web_sources_event'
import styles from './style.module.scss'

function SearchSummary (props: { searchQueries: string[] }) {
  const context = useUntrustedConversationContext()

  const handleOpenSearchQuery = React.useCallback((e: React.MouseEvent, query: string) => {
    e.preventDefault()
    context.uiHandler?.openSearchURL(query)
  }, [])

  const handleLearnMore = () => {
    context.uiHandler?.openLearnMoreAboutBraveSearchWithLeo()
  }

  const message = formatMessage(getLocale('searchQueries'), {
    placeholders: {
      $1: props.searchQueries.map((query, i, a) => (
        <React.Fragment key={i}>
          "<a className={styles.searchQueryLink} href='#' onClick={(e) => handleOpenSearchQuery(e, query)}>
            {query}
          </a>"{(i < a.length-1) ? ', ' : null}
        </React.Fragment>
      ))
    }
  })

  return (
    <div className={styles.searchSummary}>
      <Icon name="brave-icon-search-color" />
      <span>
        {message} <a className={styles.searchLearnMoreLink} href='#' onClick={handleLearnMore}>{getLocale('learnMore')}</a>
      </span>
    </div>
  )
}

export function ToolEvent(props: { event: Mojom.ToolUseEvent, isActiveEntry: boolean }) {
  const context = useUntrustedConversationContext()
  const toolUse = props.event

  const input: any | null = React.useMemo(() => {
    if (!toolUse.inputJson) {
      return null
    }
    let input
    try {
      input = JSON.parse(toolUse.inputJson)
    } catch (e) {
      return null
    }
    return input
  }, [toolUse.inputJson])

  const output: any | null = React.useMemo(() => {
    if (!toolUse.outputJson) {
      return null
    }
    let output
    try {
      output = JSON.parse(toolUse.outputJson)
    } catch (e) {
      return null
    }
    return output
  }, [toolUse.outputJson])

  if (toolUse.toolName === 'active_web_page_content_fetcher') {
    return (
      <div className={styles.toolUseRequest}>Leo is requesting access to page content
        <Button onClick={() => context.conversationHandler?.respondToToolUseRequest(toolUse.toolId, null)}>Allow</Button>
      </div>
    )
  }

  let toolText = <>{toolUse.toolName}</>

  if (toolUse.toolName === 'computer') {
    switch (input?.action) {
      case 'screenshot': {
        toolText = <>Looking at the page</>
        if (output) {
          toolText = (
            <Tooltip>
              {toolText}
              <div slot='content'><div><img className={styles.screenshotPreview} src={output?.[0]?.image_url} /></div></div>
            </Tooltip>
          )
        }
        break
      }
      case 'key': {
        toolText = <>Pressing the key: {input?.text}</>
        break
      }
      case 'type': {
        toolText = <div title={input?.text} className={styles.toolUseLongText}>Typing: "{input?.text}"</div>
        break
      }
      case 'mouse_move': {
        toolText = <>Moving the mouse</>
        break
      }
      case 'left_click': {
        toolText = <>Clicking the left mouse button</>
        break
      }
      default: {
        toolText = <>{input?.action}</>
      }
    }
  }

  if (toolUse.toolName === 'web_page_navigator') {
    toolText = <>Navigating to the URL: <span className={styles.toolUrl}>{input?.website_url}</span></>
  }

  return (
    <div className={classnames(styles.toolUse, output && styles.toolUseComplete)}>
      <div className={styles.toolUseIcon} title={toolUse.inputJson}>
        {!output && <ProgressRing />}
        {output && <Icon name="check-circle-outline" />}
      </div>
      {toolText}

    </div>
  )
}

function AssistantEvent(props: { event: Mojom.ConversationEntryEvent, hasCompletionStarted: boolean, isEntryInProgress: boolean, isActiveEntry: boolean }) {
  if (props.event.completionEvent) {
    return (
      <MarkdownRenderer
        shouldShowTextCursor={props.isEntryInProgress}
        text={props.event.completionEvent.completion}
      />
    )
  }
  if (props.event.searchStatusEvent && props.isEntryInProgress && !props.hasCompletionStarted) {
    return (
      <div className={styles.actionInProgress}><ProgressRing />Improving answer with Brave Search…</div>
    )
  }
  if (props.event.pageContentRefineEvent && props.isEntryInProgress && !props.hasCompletionStarted) {
    return (
      <div className={styles.actionInProgress}><ProgressRing />{getLocale('pageContentRefinedInProgress')}</div>
    )
  }
  else if (props.event.toolUseEvent) {
    return <ToolEvent event={props.event.toolUseEvent} isActiveEntry={props.isActiveEntry} />
  }
  // TODO(petemill): Consider displaying in-progress queries if the API
  // timing improves (or worsens for the completion events).
  // if (event.searchQueriesEvent && props.isEntryInProgress) {
  //   return (<>
  //     {event.searchQueriesEvent.searchQueries.map(query => <div className={styles.searchQuery}>Searching for <span className={styles.searchLink}><Icon name="brave-icon-search-color" /><Link href='#'>{query}</Link></span></div>)}
  //   </>)
  // }

  // Unknown events should be ignored
  return null
}

export default function AssistantResponse(props: { entry: Mojom.ConversationTurn, isEntryInProgress: boolean }) {
  const context = useUntrustedConversationContext()
  // Extract certain events which need to render at specific locations (e.g. end of the events)
  // If the entry is currently being acted on, whether it's still generating or not, the tools may
  // still be evaluating.
  const isActiveEntry = (context.conversationHistory[context.conversationHistory.length - 1] === props.entry)
  const searchQueriesEvent = props.entry.events?.find(event => event.searchQueriesEvent)?.searchQueriesEvent
  const sourcesEvent = props.entry.events?.find(event => !!event.sourcesEvent)?.sourcesEvent

  const hasCompletionStarted = !props.isEntryInProgress ||
    (props.entry.events?.some(event => event.completionEvent) ?? false)

  return (<>
  {
    props.entry.events?.map((event, i) =>
      <AssistantEvent
        key={i}
        event={event}
        hasCompletionStarted={hasCompletionStarted}
        isActiveEntry={isActiveEntry}
        isEntryInProgress={props.isEntryInProgress}
      />
    )
  }

  {
    !props.isEntryInProgress &&
    <>
      {searchQueriesEvent && <SearchSummary searchQueries={searchQueriesEvent.searchQueries} />}
      {sourcesEvent && <WebSourcesEvent sources={sourcesEvent.sources} />}
    </>
  }
  </>)
}
