/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocale, usePluralString } from '../../../context/locale_context'
import { isNewsChannelEnabled, isNewsPublisherEnabled } from '../../../api/news_api'
import { useNewsState, useNewsActions } from '../../../context/news_context'
import { SafeImage } from '../../common/safe_image'
import { CategoryIcon } from '../category_icon'
import { CategoryName } from '../category_name'

import { style } from './following_list.style'

export function FollowingList() {
  const { getString } = useLocale()
  const actions = useNewsActions()
  const channels = useNewsState((s) => s.newsChannels)
  const publishers = useNewsState((s) => s.newsPublishers)

  const enabledChannels =
    Object.values(channels).filter(isNewsChannelEnabled)
  const enabledPublishers =
    Object.values(publishers).filter(isNewsPublisherEnabled)

  const sourceCountText = usePluralString(
    'newsSourceCountText',
    enabledChannels.length + enabledPublishers.length)

  return (
    <div data-css-scope={style.scope}>
      <h4>
        {getString('newsSettingsFollowingTitle')}
        <span className='source-count'>{sourceCountText}</span>
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
                  actions.setNewsChannelEnabled(channel.channelName, false)
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
                  actions.setNewsPublisherEnabled(publisher.publisherId, false)
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
