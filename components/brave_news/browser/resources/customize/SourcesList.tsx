/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import usePromise from '$web-common/usePromise'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import * as React from 'react'
import { useBraveNews, useChannels } from '../shared/Context'
import Loading from './Loading'
import { ChannelListEntry, FeedListEntry } from './SourcesListEntry'

import { style } from './SourcesList.style'

export default function SourcesList() {
  const { subscribedPublisherIds, publishersLoaded, channelsLoaded } = useBraveNews()
  const channels = useChannels({ subscribedOnly: true })
  const isLoading = !publishersLoaded || !channelsLoaded

  const { result: sourcesCount } = usePromise(
    async () =>
      PluralStringProxyImpl.getInstance().getPluralString(
        S.BRAVE_NEWS_SOURCE_COUNT,
        subscribedPublisherIds.length + channels.length,
      ),
    [subscribedPublisherIds.length, channels.length],
  )

  return (
    <div data-css-scope={style.scope}>
      <div className='heading'>
        <span className='title'>{getLocale(S.BRAVE_NEWS_FEEDS_HEADING)}</span>
        {!isLoading && <span className='count'>{sourcesCount}</span>}
      </div>
      <div className='list'>
        {isLoading ? (
          <Loading />
        ) : (
          <>
            {channels.map((c) => (
              <ChannelListEntry key={c.channelName} channelName={c.channelName} />
            ))}
            {subscribedPublisherIds.map((p) => (
              <FeedListEntry key={p} publisherId={p} />
            ))}
          </>
        )}
      </div>
    </div>
  )
}
