import { useCallback, useEffect, useMemo, useState } from 'react'
import getBraveNewsController, { Publishers, Channels, Channel } from '.'
import { BraveNewsControllerRemote, FeedSearchResultItem, Publisher, PublisherType, UserEnabled } from '../../../../../out/Component/gen/brave/components/brave_today/common/brave_news.mojom.m'

type PublisherListener = (newValue: Publisher, oldValue: Publisher) => void
type PublishersListener = (publishers: Publishers, oldValue: Publishers) => void
type ChannelsListener = (newValue: Channels, oldValue: Channels) => void

interface PublisherFilter {
    subscribed?: boolean
    channelId?: string
    locale?: string
}

class BraveNewsApi {
    controller: BraveNewsControllerRemote

    publishersListeners: PublishersListener[] = []
    publisherListeners: Map<string, PublisherListener[]> = new Map()
    lastPublishers: Publishers = {}

    channelsListeners: ChannelsListener[] = []
    lastChannels: Channels = {}

    #locale: string

    constructor () {
        this.controller = getBraveNewsController()
        this.updateChannels()

        this.controller.getLocale().then(({ locale }) => {
            this.#locale = locale
            this.updatePublishers()
        })
    }

    getPublisher (publisherId: string) {
        return this.lastPublishers[publisherId]
    }

    getPublishers (filter?: PublisherFilter) {
        const locale = filter?.locale ?? this.#locale
        const channelId = filter?.channelId
        const subscribed = filter?.subscribed

        let publishers = Object.values(this.lastPublishers)

        publishers = publishers.filter(p =>
            // Direct sources are not tied to any locale, and should be shown for all.
            p.type === PublisherType.DIRECT_SOURCE ||
            // Otherwise, include the source if it is available in the desired locale.
            p.locales.includes(locale))
        if (channelId) { publishers = publishers.filter(p => p.channels.includes(channelId)) }
        if (subscribed !== undefined) { publishers = publishers.filter(p => this.isPublisherEnabled(p.publisherId) === subscribed) }
        return publishers
    }

    getChannels () {
        return Object.values(this.lastChannels)
    }

    getChannel (channelId: string) {
        return this.lastChannels[channelId]
    }

    async setPublisherPref (publisherId: string, status: UserEnabled) {
        const newValue = {
            ...this.lastPublishers,
            [publisherId]: {
                ...this.lastPublishers[publisherId],
                userEnabledStatus: status
            }
        }

        // We completely remove direct feeds when setting the UserEnabled status
        // to DISABLED.
        if (this.isDirectFeed(publisherId) && status === UserEnabled.DISABLED) { delete newValue[publisherId] }

        this.controller.setPublisherPref(publisherId, status)
        this.updatePublishers(newValue)
    }

    async subscribeToDirectFeed (feedUrl: string) {
        const { publishers } = await this.controller.subscribeToNewDirectFeed({ url: feedUrl })
        this.updatePublishers(publishers)
    }

    setPublisherSubscribed (publisherId: string, enabled: boolean) {
        this.setPublisherPref(publisherId, enabled ? UserEnabled.ENABLED : UserEnabled.DISABLED)
    }

    isPublisherEnabled (publisherId: string) {
        const publisher = this.lastPublishers[publisherId]
        if (!publisher) return false

        // Direct Sources are enabled if they're available.
        if (publisher.type === PublisherType.DIRECT_SOURCE) return true

        return publisher.isEnabled &&
            publisher.userEnabledStatus === UserEnabled.NOT_MODIFIED ||
            publisher.userEnabledStatus === UserEnabled.ENABLED
    }

    isDirectFeed (publisherId: string) {
        const publisher = this.lastPublishers[publisherId]
        if (!publisher) return false
        return publisher.type === PublisherType.DIRECT_SOURCE
    }

    async setChannelSubscribed (channelId: string, subscribed: boolean) {
        // While we're waiting for the new channels to come back, speculatively
        // update them, so the UI has instant feedback.
        this.updateChannels({
            ...this.lastChannels,
            [channelId]: {
                ...this.lastChannels[channelId],
                subscribed
            }
        })

        // Then, once we receive the actual update, apply it.
        const { updated } = await this.controller.setChannelSubscribed(channelId, subscribed)
        this.updateChannels({
            ...this.lastChannels,
            [channelId]: updated
        })
    }

    async updatePublishers (newPublishers?: Publishers) {
        if (!newPublishers) { ({ publishers: newPublishers } = await this.controller.getPublishers()) }

        const oldValue = this.lastPublishers
        this.lastPublishers = newPublishers!

        this.notifyPublishersListeners(newPublishers!, oldValue)
    }

    async updateChannels (newChannels?: Channels) {
        if (!newChannels) { ({ channels: newChannels } = await this.controller.getChannels()) }

        const oldValue = this.lastChannels
        this.lastChannels = newChannels!

        this.notifyChannelsListeners(this.lastChannels, oldValue)
    }

    addPublishersListener (listener: PublishersListener) {
        this.publishersListeners.push(listener)
    }

    removePublishersListener (listener: PublishersListener) {
        const index = this.publishersListeners.indexOf(listener)
        this.publishersListeners.splice(index, 1)
    }

    addChannelsListener (listener: ChannelsListener) {
        this.channelsListeners.push(listener)
    }

    removeChannelsListener (listener: ChannelsListener) {
        const index = this.channelsListeners.indexOf(listener)
        this.channelsListeners.splice(index, 1)
    }

    addPublisherListener (publisherId: string, listener: PublisherListener) {
        if (!this.publisherListeners.has(publisherId)) { this.publisherListeners.set(publisherId, []) }

        this.publisherListeners.get(publisherId)!.push(listener)
    }

    removePublisherListener (publisherId: string, listener: PublisherListener) {
        const listeners = this.publisherListeners.get(publisherId)
        if (!listeners) return

        const index = listeners.indexOf(listener)
        listeners.splice(index, 1)

        if (listeners.length === 0) { this.publisherListeners.delete(publisherId) }
    }

    notifyPublishersListeners (newValue: Publishers, oldValue: Publishers) {
        for (const listener of this.publishersListeners) { listener(newValue, oldValue) }

        this.notifyPublisherListeners(newValue, oldValue)
    }

    notifyPublisherListeners (newValue: Publishers, oldValue: Publishers) {
        for (const publisher of Object.values(newValue)) {
            const oldPublisher = oldValue[publisher.publisherId]
            if (publisher === oldPublisher) continue

            for (const listener of this.publisherListeners.get(publisher.publisherId) ?? []) { listener(publisher, oldPublisher) }
        }
    }

    notifyChannelsListeners (newValue: Channels, oldValue: Channels) {
        for (const listener of this.channelsListeners) { listener(newValue, oldValue) }
    }
}

export const api = new BraveNewsApi()

export const useChannels = (options: { subscribedOnly: boolean } = { subscribedOnly: false }) => {
    const [channels, setChannels] = useState<Channel[]>([])
    useEffect(() => {
        const handler = () => setChannels(api.getChannels())
        handler()

        api.addChannelsListener(handler)
        return () => api.removeChannelsListener(handler)
    }, [])

    const filteredChannels = useMemo(() => options.subscribedOnly ? channels.filter(c => c.subscribed) : channels, [options.subscribedOnly, channels])
    return filteredChannels
}

export const useChannelSubscribed = (channelId: string) => {
    const [channel, setChannel] = useState(api.getChannel(channelId))
    useEffect(() => {
        const handler = () => setChannel(api.getChannel(channelId))
        handler()
        api.addChannelsListener(handler)

        return () => api.removeChannelsListener(handler)
    }, [channelId])

    const setSubscribed = (subscribed: boolean) => api.setChannelSubscribed(channelId, subscribed)

    return {
        subscribed: channel.subscribed,
        setSubscribed
    }
}

export const usePublishers = (options?: PublisherFilter) => {
    const [publishers, setPublishers] = useState<Publisher[]>([])
    useEffect(() => {
        const handler = () => setPublishers(api.getPublishers(options))
        handler()

        api.addPublishersListener(handler)
        return () => {
            api.removePublishersListener(handler)
        }
    }, [options?.subscribed, options?.channelId, options?.locale])

    const sorted = useMemo(() => publishers.sort((a, b) => a.publisherName.localeCompare(b.publisherName)), [publishers])
    return sorted
}

export const usePublisher = (publisherId: string) => {
    const [publisher, setPublisher] = useState(api.getPublisher(publisherId))

    useEffect(() => {
        const handler: PublisherListener = newValue => {
            setPublisher(newValue)
        }
        api.addPublisherListener(publisherId, handler)

        return () => {
            api.removePublisherListener(publisherId, handler)
        }
    }, [publisherId])

    return publisher
}

export const usePublisherSubscribed = (publisherId: string) => {
    // We depend on the publisher and the active channels because either
    // changing could affect out subscription status.
    const publisher = usePublisher(publisherId)
    const channels = useChannels()

    const setSubscribed = useCallback((enabled: boolean) => {
        api.setPublisherSubscribed(publisherId, enabled)
    }, [publisherId])

    const subscribed = api.isPublisherEnabled(publisherId)
    const inChannel = useMemo(() => publisher.channels.some(c => api.getChannel(c)?.subscribed), [publisher, channels])
    return {
        channelSubscribed: inChannel,
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

    useEffect(() => {
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
