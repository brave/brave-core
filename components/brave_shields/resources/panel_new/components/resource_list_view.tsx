/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { style } from './resource_list_view.style'

interface Props {
  urls: string[]
  actionText?: string
  actionHandler?: (url: string) => void
}

export function ResourceListView(props: Props) {
  const originMap = React.useMemo(() => groupByOrigin(props.urls), [props.urls])
  return (
    <div data-css-scope={style.scope}>
      <div className='list'>
        {[...originMap.entries()].map(([origin, urls]) => (
          <ResourceGroup
            key={origin}
            origin={origin}
            urls={[...urls.values()]}
            actionText={props.actionText}
            actionHandler={props.actionHandler}
          />
        ))}
      </div>
    </div>
  )
}

interface ResourceGroupProps {
  origin: string
  urls: string[]
  actionText?: string
  actionHandler?: (url: string) => void
}

function ResourceGroup(props: ResourceGroupProps) {
  const [expanded, setExpanded] = React.useState(false)
  const { origin, urls } = props

  if (urls.length === 0) {
    return null
  }

  if (urls.length === 1) {
    return (
      <ResourceEntry
        icon='dot'
        text={origin + urls[0]}
        url={origin + urls[0]}
        actionText={props.actionText}
        actionHandler={props.actionHandler}
      />
    )
  }

  return (
    <div
      className='group'
      data-expanded={expanded}
    >
      <ResourceEntry
        icon={expanded ? 'minus' : 'plus-add'}
        onIconClick={() => setExpanded(!expanded)}
        text={origin}
        url={origin}
        actionText={props.actionText}
        actionHandler={props.actionHandler}
      />
      <div className='list'>
        {urls.map((url) => (
          <ResourceEntry
            key={url}
            icon={'dot'}
            text={url}
            url={origin + url}
            actionText={props.actionText}
            actionHandler={props.actionHandler}
          />
        ))}
      </div>
    </div>
  )
}

interface ResourceEntryProps {
  text: string
  url: string
  icon: 'plus-add' | 'minus' | 'dot'
  onIconClick?: () => void
  actionText?: string
  actionHandler?: (url: string) => void
}

function ResourceEntry(props: ResourceEntryProps) {
  return (
    <div className='resource'>
      {props.onIconClick ? (
        <button onClick={props.onIconClick}>
          <Icon name={props.icon} />
        </button>
      ) : (
        <Icon name={props.icon} />
      )}
      <div className='text'>{props.text}</div>
      {props.actionHandler && props.actionText && (
        <button
          className='action'
          onClick={props.actionHandler.bind(undefined, props.url)}
        >
          {props.actionText}
        </button>
      )}
    </div>
  )
}

function groupByOrigin(urls: string[]) {
  const map = new Map<string, Set<string>>()

  for (const url of urls) {
    const { origin, pathname, search } = new URL(url)
    let items = map.get(origin)
    if (!items) {
      items = new Set()
      map.set(origin, items)
    }
    items.add(pathname + search)
  }

  return map
}
