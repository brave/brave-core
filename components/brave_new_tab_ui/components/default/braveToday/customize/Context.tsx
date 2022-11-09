// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useCallback, useMemo, useState } from 'react'
import { Channels, Publisher, Publishers, PublisherType } from '../../../../api/brave_news'
import { api, isPublisherEnabled } from '../../../../api/brave_news/news'
import Modal from './Modal'

// Leave possibility for more pages open.
type NewsPage = null
  | 'news'
  | 'suggestions'
  | 'popular'

interface BraveNewsContext {
  customizePage: NewsPage
  setCustomizePage: (page: NewsPage) => void
  channels: Channels
  // All global Publishers
  publishers: Publishers
  sortedPublishers: Publisher[]
  // Publishers to offer the user (i.e. from current locale)
  filteredPublisherIds: string[]
  // Publishers the user is directly subscribed to
  subscribedPublisherIds: string[]
  // Publishers to suggest to the user.
  suggestedPublisherIds: string[]
  updateSuggestedPublisherIds: () => void
}

export const BraveNewsContext = React.createContext<BraveNewsContext>({
  customizePage: null,
  setCustomizePage: () => { },
  publishers: {},
  sortedPublishers: [],
  filteredPublisherIds: [],
  subscribedPublisherIds: [],
  channels: {},
  suggestedPublisherIds: [],
  updateSuggestedPublisherIds: () => {}
})

export function BraveNewsContextProvider (props: { children: React.ReactNode }) {
  const [customizePage, setCustomizePage] = useState<NewsPage>(null)
  const [channels, setChannels] = useState<Channels>({})
  const [publishers, setPublishers] = useState<Publishers>({})
  const [suggestedPublisherIds, setSuggestedPublisherIds] = useState<string[]>([])

  React.useEffect(() => {
    const handler = () => setChannels(api.getChannels())
    handler()

    api.addChannelsListener(handler)
    return () => api.removeChannelsListener(handler)
  }, [])

  const updateSuggestedPublisherIds = useCallback(async () => {
    setSuggestedPublisherIds([])
    const { suggestedPublisherIds } = await api.controller.getSuggestedPublisherIds()
    setSuggestedPublisherIds(suggestedPublisherIds)
  }, [])

  React.useEffect(() => {
    const handler = () => setPublishers(api.getPublishers())
    handler()

    api.addPublishersListener(handler)
    return () => api.removePublishersListener(handler)
  }, [])

  const sortedPublishers = useMemo(() =>
    Object.values(publishers)
      .sort((a, b) => a.publisherName.localeCompare(b.publisherName)),
    [publishers])

  const filteredPublisherIds = useMemo(() =>
    sortedPublishers
      .filter(p => p.type === PublisherType.DIRECT_SOURCE ||
        p.locales.some(l => l.locale === api.locale))
      .map(p => p.publisherId),
    [sortedPublishers])

  const subscribedPublisherIds = useMemo(() =>
    sortedPublishers.filter(isPublisherEnabled).map(p => p.publisherId),
    [sortedPublishers])

  const context = useMemo<BraveNewsContext>(() => ({
    customizePage,
    setCustomizePage,
    channels,
    publishers,
    suggestedPublisherIds,
    sortedPublishers,
    filteredPublisherIds,
    subscribedPublisherIds,
    updateSuggestedPublisherIds
  }), [customizePage, channels, publishers, suggestedPublisherIds, updateSuggestedPublisherIds])

  return <BraveNewsContext.Provider value={context}>
    {props.children}
    <Modal />
  </BraveNewsContext.Provider>
}

export const useBraveNews = () => {
  return React.useContext(BraveNewsContext)
}

export const useChannels = (options: { subscribedOnly: boolean } = { subscribedOnly: false }) => {
  const { channels } = useBraveNews()
  return useMemo(() => Object.values(channels)
    .filter(c => c.subscribedLocales.length || !options.subscribedOnly), [channels, options.subscribedOnly])
}

/**
 * Determines whether the channel is subscribed in the current locale.
 * @param channelName The channel
 * @returns A getter & setter for whether the channel is subscribed
 */
export const useChannelSubscribed = (channelName: string) => {
  const { channels } = useBraveNews()
  const subscribed = useMemo(() => channels[channelName]?.subscribedLocales.includes(api.locale) ?? false, [channels[channelName]])
  const setSubscribed = React.useCallback((subscribed: boolean) => {
    api.setChannelSubscribed(channelName, subscribed)
  }, [channelName])

  return {
    subscribed,
    setSubscribed
  }
}

export const usePublisher = (publisherId: string) => {
  const { publishers } = useBraveNews()
  return useMemo(() => publishers[publisherId], [publishers[publisherId]])
}

export const usePublisherFollowed = (publisherId: string) => {
  const publisher = usePublisher(publisherId)

  const followed = isPublisherEnabled(publisher)
  const setFollowed = useCallback((followed: boolean) => api.setPublisherFollowed(publisherId, followed), [publisherId])

  return {
    followed,
    setFollowed
  }
}
