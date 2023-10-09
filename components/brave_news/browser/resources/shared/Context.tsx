// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useCallback, useMemo, useState, useEffect } from 'react'
import getBraveNewsController, { Channels, Publisher, Publishers, PublisherType, isPublisherEnabled, Configuration } from './api'
import { PublishersCachingWrapper } from './publishersCache'
import { ChannelsCachingWrapper } from './channelsCache'
import { ConfigurationCachingWrapper } from './configurationCache'

// Leave possibility for more pages open.
type NewsPage = null
  | 'news'
  | 'suggestions'
  | 'popular'

interface BraveNewsContext {
  locale: string
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
  isOptInPrefEnabled: boolean | undefined
  isShowOnNTPPrefEnabled: boolean | undefined
  toggleBraveNewsOnNTP: (enabled: boolean) => void
}

export const BraveNewsContext = React.createContext<BraveNewsContext>({
  locale: '',
  customizePage: null,
  setCustomizePage: () => { },
  publishers: {},
  sortedPublishers: [],
  filteredPublisherIds: [],
  subscribedPublisherIds: [],
  channels: {},
  suggestedPublisherIds: [],
  updateSuggestedPublisherIds: () => { },
  isOptInPrefEnabled: undefined,
  isShowOnNTPPrefEnabled: undefined,
  toggleBraveNewsOnNTP: (enabled: boolean) => { }
})

const publishersCache = new PublishersCachingWrapper()
const channelsCache = new ChannelsCachingWrapper()
const configurationCache = new ConfigurationCachingWrapper()

export function BraveNewsContextProvider(props: { children: React.ReactNode }) {
  const [locale, setLocale] = useState('')
  const [customizePage, setCustomizePage] = useState<NewsPage>(null)
  const [channels, setChannels] = useState<Channels>({})
  const [publishers, setPublishers] = useState<Publishers>({})
  const [suggestedPublisherIds, setSuggestedPublisherIds] = useState<string[]>([])
  const [configuration, setConfiguration] = useState<Configuration>(configurationCache.value)

  // Get the default locale on load.
  useEffect(() => {
    getBraveNewsController().getLocale().then(({ locale }) => setLocale(locale))
  }, [configuration.isOptedIn, configuration.showOnNTP])

  React.useEffect(() => {
    const handler = (channels: Channels) => setChannels(channels)

    channelsCache.addListener(handler)
    return () => channelsCache.removeListener(handler)
  }, [])

  React.useEffect(() => {
    configurationCache.addListener(setConfiguration)
    return () => configurationCache.removeListener(setConfiguration)
  }, [])

  const updateSuggestedPublisherIds = useCallback(async () => {
    setSuggestedPublisherIds([])
    const { suggestedPublisherIds } = await getBraveNewsController().getSuggestedPublisherIds()
    setSuggestedPublisherIds(suggestedPublisherIds)
  }, [])

  React.useEffect(() => {
    const handler = (publishers: Publishers) => setPublishers(publishers)
    publishersCache.addListener(handler)
    return () => { publishersCache.removeListener(handler) }
  }, [])

  const sortedPublishers = useMemo(() =>
    Object.values(publishers)
      .sort((a, b) => a.publisherName.localeCompare(b.publisherName)),
    [publishers])

  const filteredPublisherIds = useMemo(() =>
    sortedPublishers
      .filter(p => p.type === PublisherType.DIRECT_SOURCE ||
        p.locales.some(l => l.locale === locale))
      .map(p => p.publisherId),
    [sortedPublishers, locale])

  const subscribedPublisherIds = useMemo(() =>
    sortedPublishers.filter(isPublisherEnabled).map(p => p.publisherId),
    [sortedPublishers])

  const toggleBraveNewsOnNTP = (shouldEnable: boolean) => {
    if (shouldEnable) {
      configurationCache.set({ isOptedIn: true, showOnNTP: true })
      return
    }
    configurationCache.set({ showOnNTP: false })
  }

  const context = useMemo<BraveNewsContext>(() => ({
    locale,
    customizePage,
    setCustomizePage,
    channels,
    publishers,
    suggestedPublisherIds,
    sortedPublishers,
    filteredPublisherIds,
    subscribedPublisherIds,
    updateSuggestedPublisherIds,
    isOptInPrefEnabled: configuration.isOptedIn,
    isShowOnNTPPrefEnabled: configuration.showOnNTP,
    toggleBraveNewsOnNTP
  }), [customizePage, channels, publishers, suggestedPublisherIds, updateSuggestedPublisherIds, configuration, toggleBraveNewsOnNTP])

  return <BraveNewsContext.Provider value={context}>
    {props.children}
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
  const { channels, locale } = useBraveNews()
  const subscribed = useMemo(() => channels[channelName]?.subscribedLocales.includes(locale) ?? false,
    [channels[channelName], locale])
  const setSubscribed = React.useCallback((subscribed: boolean) => {
    channelsCache.setChannelSubscribed(locale, channelName, subscribed)
  }, [channelName, locale])

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
  const setFollowed = useCallback((followed: boolean) => publishersCache.setPublisherFollowed(publisherId, followed), [publisherId])

  return {
    followed,
    setFollowed
  }
}
