/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useChannelSubscribed } from '../../../../../../components/brave_news/browser/resources/shared/Context'
import { SafeImage } from '../../common/safe_image'
import { inlineCSSVars } from '../../../lib/inline_css_vars'
import { CategoryIcon } from '../category_icon'
import { CategoryName } from '../category_name'
import classNames from '$web-common/classnames'

import { style } from './channel_card.style'

interface Props {
  background?: string
  enabled: boolean
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
      <div className={classNames({cover: true, enabled: props.enabled })}>
        {image}
        <button onClick={props.onToggle}>
          <Icon name={props.enabled ? 'minus' : 'plus-add'} />
        </button>
      </div>
      {props.name}
    </div>
  )
}

export function ChannelCard(props: { channelName: string }) {
  const { subscribed, setSubscribed } = useChannelSubscribed(props.channelName)
  return (
    <SourceCard
      image={<CategoryIcon category={props.channelName} />}
      name={<CategoryName category={props.channelName} />}
      enabled={subscribed}
      onToggle={() => setSubscribed(!subscribed)}
    />
  )
}
