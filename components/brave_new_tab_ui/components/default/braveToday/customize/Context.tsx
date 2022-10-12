import * as React from 'react'
import { useCallback, useMemo, useState } from 'react'
import { Channels, Publisher, Publishers, PublisherType } from '../../../../api/brave_news'
import { api, isPublisherEnabled } from '../../../../api/brave_news/news'
import Modal from './Modal'

// Leave possibility for more pages open.
type NewsPage = null
  | 'news'

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
}

export const BraveNewsContext = React.createContext<BraveNewsContext>({
  customizePage: null,
  setCustomizePage: () => { },
  publishers: {},
  sortedPublishers: [],
  filteredPublisherIds: [],
  subscribedPublisherIds: [],
  channels: {}
})

export function BraveNewsContextProvider (props: { children: React.ReactNode }) {
  const [customizePage, setCustomizePage] = useState<NewsPage>(null)
  const [channels, setChannels] = useState<Channels>({})
  const [publishers, setPublishers] = useState<Publishers>({})

  React.useEffect(() => {
    const handler = () => setChannels(api.getChannels())
    handler()

    api.addChannelsListener(handler)
    return () => api.removeChannelsListener(handler)
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
      .filter(p => p.type === PublisherType.DIRECT_SOURCE || p.locales.includes(api.locale))
      .map(p => p.publisherId),
    [sortedPublishers])

  const subscribedPublisherIds = useMemo(() =>
    sortedPublishers.filter(isPublisherEnabled).map(p => p.publisherId),
    [sortedPublishers])

  const context = useMemo(() => ({
    customizePage,
    setCustomizePage,
    channels,
    publishers,
    sortedPublishers,
    filteredPublisherIds,
    subscribedPublisherIds
  }), [customizePage, channels, publishers])

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
    .filter(c => c.subscribed || !options.subscribedOnly), [channels, options.subscribedOnly])
}

export const useChannelSubscribed = (channelId: string) => {
  const { channels } = useBraveNews()
  const subscribed = useMemo(() => channels[channelId]?.subscribed ?? false, [channels[channelId]])
  const setSubscribed = React.useCallback((subscribed: boolean) => {
    api.setChannelSubscribed(channelId, subscribed)
  }, [channelId])

  return {
    subscribed,
    setSubscribed
  }
}

export const usePublisher = (publisherId: string) => {
  const { publishers } = useBraveNews()
  return useMemo(() => publishers[publisherId], [publishers[publisherId]])
}

export const usePublisherSubscribed = (publisherId: string) => {
  const publisher = usePublisher(publisherId)

  const subscribed = isPublisherEnabled(publisher)
  const setSubscribed = useCallback((subscribed: boolean) => api.setPublisherSubscribed(publisherId, subscribed), [publisherId])

  return {
    subscribed,
    setSubscribed
  }
}
