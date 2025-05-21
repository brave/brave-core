/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import {
  Publisher,
  Channel,
  isNewsPublisherEnabled,
  isNewsChannelEnabled,
} from '../../state/news_state'

import { useNewsActions } from '../../context/news_context'
import { SafeImage } from '../common/safe_image'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { getKeyedColor } from '../../lib/keyed_colors'
import { CategoryIcon } from './category_icon'
import { CategoryName } from './category_name'
import classNames from '$web-common/classnames'

import { style } from './source_card.style'

interface Props {
  background?: string
  enabled: boolean
  loading?: boolean
  image: React.ReactNode
  name: React.ReactNode
  onToggle: () => void
}

export function SourceCard(props: Props) {
  const background = props.background ?? ''

  let { image } = props
  if (typeof image === 'string') {
    image = <SafeImage src={image} />
  }

  return (
    <div
      data-css-scope={style.scope}
      style={inlineCSSVars({ '--cover-background': background })}
    >
      <div className={classNames({ 'cover': true, 'enabled': props.enabled })}>
        {image}
        <button
          disabled={props.loading}
          onClick={props.onToggle}
        >
          <Icon name={props.enabled ? 'minus' : 'plus-add'} />
        </button>
      </div>
      {props.name}
    </div>
  )
}

export function PublisherSourceCard(props: { publisher: Publisher }) {
  const actions = useNewsActions()
  const { publisher } = props
  const enabled = isNewsPublisherEnabled(publisher)
  const background =
    publisher.backgroundColor
    || getKeyedColor(publisher.feedSource.url || publisher.publisherId)

  return (
    <SourceCard
      background={background}
      image={publisher.coverUrl?.url}
      name={publisher.publisherName}
      enabled={enabled}
      onToggle={() => {
        actions.setPublisherEnabled(publisher.publisherId, !enabled)
      }}
    />
  )
}

export function DirectFeedSourceCard(props: { title: string; url: string }) {
  const actions = useNewsActions()
  const [loading, setLoading] = React.useState(false)
  return (
    <SourceCard
      background={getKeyedColor(props.url)}
      name={props.title}
      image={null}
      enabled={false}
      loading={loading}
      onToggle={() => {
        setLoading(true)
        actions.subscribeToNewDirectFeed(props.url).then(() => {
          setLoading(false)
        })
      }}
    />
  )
}

export function ChannelSourceCard(props: { channel: Channel }) {
  const actions = useNewsActions()
  const { channel } = props
  const enabled = isNewsChannelEnabled(channel)
  return (
    <SourceCard
      key={channel.channelName}
      image={<CategoryIcon category={channel.channelName} />}
      name={<CategoryName category={channel.channelName} />}
      enabled={enabled}
      onToggle={() => {
        actions.setChannelSubscribed(channel.channelName, !enabled)
      }}
    />
  )
}
