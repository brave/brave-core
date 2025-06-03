/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getString } from '../../../lib/strings'
import { isNewsChannelEnabled, isNewsPublisherEnabled } from '../../../state/news_state'
import { useNewsState, useNewsActions } from '../../../context/news_context'
import { SafeImage } from '../../common/safe_image'
import { PluralString } from '../../common/plural_string'
import { CategoryIcon } from '../category_icon'
import { CategoryName } from '../category_name'

import { style } from './following_list.style'

export function FollowingList() {
  const actions = useNewsActions()
  const channels = useNewsState((s) => s.channels)
  const publishers = useNewsState((s) => s.publishers)

  const enabledChannels = React.useMemo(() => {
    return Object.values(channels).filter(isNewsChannelEnabled)
  }, [channels])

  const enabledPublishers = React.useMemo(() => {
    return Object.values(publishers).filter(isNewsPublisherEnabled)
  }, [publishers])

  return (
    <div data-css-scope={style.scope}>
      <h4>
        {getString('newsSettingsFollowingTitle')}
        <span className='source-count'>
          <PluralString
            stringKey='newsSourceCountText'
            count={enabledChannels.length + enabledPublishers.length}
          />
        </span>
      </h4>
      <div className='sources'>
        {
          enabledChannels.map((channel) => (
            <div key={channel.channelName}>
              <CategoryIcon category={channel.channelName} />
              <span className='name'>
                <CategoryName category={channel.channelName} />
              </span>
              <button
                onClick={() => {
                  actions.setChannelSubscribed(channel.channelName, false)
                }}
              >
                {getString('newsUnfollowButtonLabel')}
              </button>
            </div>
          ))
        }
        {
          enabledPublishers.map((publisher) => (
            <div key={publisher.publisherId}>
              <SafeImage src={publisher.coverUrl?.url ?? ''} />
              <span className='name'>
                {publisher.publisherName}
              </span>
              <button
                onClick={() => {
                  actions.setPublisherEnabled(publisher.publisherId, false)
                }}
              >
                {getString('newsUnfollowButtonLabel')}
              </button>
            </div>
          ))
        }
      </div>
    </div>
  )
}
