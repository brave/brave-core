// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/nala/react/icon'
import createSanitizedImageUrl from '$web-common/create_sanitized_image_url'
import { getLocale, formatLocale } from '$web-common/locale'
import * as mojom from '../../../common/mojom'
import styles from './web_sources_event.module.scss'

const UNEXPANDED_SOURCES_COUNT = 4

function WebSource(props: { source: mojom.WebSource; citationNumber: number }) {
  const { source } = props

  const faviconSrc = source.faviconUrl.url.startsWith('chrome-untrusted://')
    ? source.faviconUrl.url
    : createSanitizedImageUrl(source.faviconUrl.url)

  const url = new URL(source.url.url)
  const content = (
    <>
      <span className={styles.favicon}>
        <img src={faviconSrc} />
      </span>
      {props.citationNumber} - {url.hostname}
    </>
  )

  // Sources open as ordinary links, like citations, so that the browser opens
  // them as a link from this frame. Only https sources are linked.
  return (
    <li>
      {url.protocol === 'https:' ? (
        <a
          className={styles.source}
          title={source.title}
          href={url.href}
          target='_blank'
          rel='noopener noreferrer'
        >
          {content}
        </a>
      ) : (
        <span
          className={styles.source}
          title={source.title}
        >
          {content}
        </span>
      )}
    </li>
  )
}

export default function WebSourcesEvent(props: { sources: mojom.WebSource[] }) {
  const [isExpanded, setIsExpanded] = React.useState(false)

  const unhiddenSources = props.sources?.slice(0, UNEXPANDED_SOURCES_COUNT)
  const hiddenSources = props.sources?.slice(UNEXPANDED_SOURCES_COUNT)

  return (
    <div
      className={styles.sources}
      data-testid='web-sources-event'
    >
      <h4>{getLocale(S.CHAT_UI_SOURCES)}</h4>
      <ul>
        {unhiddenSources.map((source, index) => (
          <WebSource
            key={source.url.url}
            source={source}
            citationNumber={index + 1}
          />
        ))}
        {!isExpanded && hiddenSources.length > 0 && (
          <li>
            <button
              className={`${styles.source} ${styles.expand}`}
              name='expand'
              onClick={() => setIsExpanded(true)}
            >
              <Icon
                className={styles.expandIcon}
                name='plus-add'
              />
              {formatLocale(S.CHAT_UI_EXPAND_SOURCES, {
                $1: hiddenSources.length,
              })}
            </button>
          </li>
        )}
        {isExpanded
          && hiddenSources.map((source, index) => (
            <WebSource
              key={source.url.url}
              source={source}
              citationNumber={index + UNEXPANDED_SOURCES_COUNT + 1}
            />
          ))}
      </ul>
    </div>
  )
}
