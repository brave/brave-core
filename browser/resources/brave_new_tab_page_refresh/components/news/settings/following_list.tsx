/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useBraveNews, useChannelSubscribed, usePublisherFollowed } from '../../../../../../components/brave_news/browser/resources/shared/Context'
import { isPublisherEnabled } from '../../../../../../components/brave_news/browser/resources/shared/api'
import { getString } from '../../../lib/strings'
import { SafeImage } from '../../common/safe_image'
import { PluralString } from '../../common/plural_string'
import { CategoryIcon } from '../category_icon'
import { CategoryName } from '../category_name'

import { style } from './following_list.style'

export function FollowingList() {
  const { channels, publishers } = useBraveNews()

  const enabledChannels = React.useMemo(() => {
    return Object.values(channels)
      .filter((channel) => channel.subscribedLocales.length > 0)
  }, [channels])

  const enabledPublishers = React.useMemo(() => {
    return Object.values(publishers).filter(isPublisherEnabled)
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
              <ChannelUnfollowButton channelName={channel.channelName} />
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
              <PublisherUnfollowButton publisherId={publisher.publisherId} />
            </div>
          ))
        }
      </div>
    </div>
  )
}

function ChannelUnfollowButton(props: { channelName: string }) {
  const { setSubscribed } = useChannelSubscribed(props.channelName)
  return (
    <button onClick={() => setSubscribed(false)}>
      {getString('newsUnfollowButtonLabel')}
    </button>
  )
}

function PublisherUnfollowButton(props: { publisherId: string }) {
  const { setFollowed } = usePublisherFollowed(props.publisherId)
  return (
    <button onClick={() => setFollowed(false) }>
      {getString('newsUnfollowButtonLabel')}
    </button>
  )
}
