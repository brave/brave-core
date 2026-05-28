// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './tab_sources_event.module.scss'

interface TabSource {
  tab_id: number
  title: string
  url: string
}

function TabSourceCard(props: { source: TabSource }) {
  const context = useUntrustedConversationContext()
  const { source } = props

  const host = (() => {
    try {
      return new URL(source.url).hostname
    } catch {
      return source.url
    }
  })()

  const faviconSrc =
    `chrome-untrusted://favicon2?size=32&pageUrl=`
    + encodeURIComponent(source.url)

  const handleClick = () => {
    context.conversationHandler?.switchToTab(source.tab_id)
  }

  return (
    <li>
      <button
        title={source.title || source.url}
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

function parseSources(artifacts: Mojom.ToolArtifact[] | null | undefined) {
  if (!artifacts) {
    return []
  }
  const result: TabSource[] = []
  for (const artifact of artifacts) {
    if (artifact.type !== Mojom.TAB_SOURCES_ARTIFACT_TYPE) {
      continue
    }
    try {
      const parsed = JSON.parse(artifact.contentJson) as {
        sources?: TabSource[]
      }
      if (Array.isArray(parsed.sources)) {
        result.push(...parsed.sources)
      }
    } catch {
      // Ignore malformed artifact content.
    }
  }
  return result
}

export default function TabSourcesEvent(props: {
  artifacts: Mojom.ToolArtifact[] | null | undefined
}) {
  const sources = React.useMemo(
    () => parseSources(props.artifacts),
    [props.artifacts],
  )
  if (sources.length === 0) {
    return null
  }
  return (
    <div
      className={styles.tabSources}
      data-testid='tab-sources-event'
    >
      <ul>
        {sources.map((source) => (
          <TabSourceCard
            key={source.tab_id}
            source={source}
          />
        ))}
      </ul>
    </div>
  )
}
