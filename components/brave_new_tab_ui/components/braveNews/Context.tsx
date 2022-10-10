import * as React from 'react'
import { useCallback, useMemo, useState } from 'react'
import { Channels, FeedSearchResultItem, Publishers, PublisherType } from '../../api/brave_news'
import { api, isPublisherEnabled } from '../../api/brave_news/news'
import Modal from './Modal'

// Leave possibility for more pages open.
type NewsPage = null
    | 'news'

interface BraveNewsContext {
    customizePage: NewsPage
    setCustomizePage: (page: NewsPage) => void
    channels: Channels
    publishers: Publishers
}

export const BraveNewsContext = React.createContext<BraveNewsContext>({
    customizePage: null,
    setCustomizePage: () => { },
    publishers: {},
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

    const context = useMemo(() => ({
        customizePage,
        setCustomizePage,
        channels,
        publishers
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

interface PublisherFilter {
    subscribed?: boolean
    channelId?: string
    locale?: string
}

export const usePublishers = (options?: PublisherFilter) => {
    const { publishers } = useBraveNews()

    const locale = options?.locale ?? api.locale
    const sorted = useMemo(() => Object.values(publishers)
        .sort((a, b) => a.publisherName.localeCompare(b.publisherName))
        .filter(p => p.type === PublisherType.DIRECT_SOURCE || p.locales.includes(locale))
        .filter(p => options?.channelId === undefined || p.channels.includes(options?.channelId))
        .filter(p => options?.subscribed === undefined || isPublisherEnabled(p) === options?.subscribed),
        [options?.channelId, options?.locale, options?.subscribed, publishers])

    return sorted
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

export const useDirectFeedResults = (query: string) => {
    // We're not interested in casing.
    query = query.toLocaleLowerCase()

    const [directResults, setDirectResults] = useState<FeedSearchResultItem[]>([])
    const [loading, setLoading] = useState(false)

    const publishers = usePublishers()

    // Filter out any direct results we already have a publisher for.
    const filteredDirectResults = useMemo(() => directResults.filter(r => !publishers.some(p => p.feedSource.url === r.feedUrl.url)), [directResults, publishers])

    React.useEffect(() => {
        setDirectResults([])

        let cancelled = false
        let url: URL | undefined
        try { url = new URL(query) } catch { }
        if (!url) return

        setLoading(true)

        api.controller.findFeeds({ url: url.toString() }).then(({ results }) => {
            if (cancelled) return

            setLoading(false)
            setDirectResults(results)
        })
        return () => {
            cancelled = true
        }
    }, [query])

    return {
        loading,
        directResults: filteredDirectResults
    }
}
