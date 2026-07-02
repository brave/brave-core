// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  Channel,
  Publisher,
  PublisherType,
  isDirectFeed,
  isPublisherEnabled
} from '../shared/api'

// OPML (https://opml.org/spec2.opml) is the de-facto standard for exchanging
// feed subscription lists. We use standard `type="rss"` outlines for feeds so
// the file is portable to other readers, plus two Brave-specific extensions
// that other readers ignore but that let subscriptions round-trip losslessly:
//  - `type="brave-channel"`: a Brave News topic channel (has no feed URL).
//  - `type="brave-publisher"`: a Brave News publisher that has no feed URL and
//    can only be resolved by its publisher id.
// Every combined-source publisher outline also carries `bravePublisherId` so it
// re-subscribes to the exact publisher on import, even when a feed URL matches
// nothing (or the publisher has no feed URL at all).
const BRAVE_CHANNEL_TYPE = 'brave-channel'
const BRAVE_PUBLISHER_TYPE = 'brave-publisher'
const BRAVE_PUBLISHER_ID_ATTR = 'bravePublisherId'

export interface OpmlItem {
  title: string
  xmlUrl?: string
  htmlUrl?: string
  // The Brave News publisher id, when the source is a known publisher. Absent
  // for direct feeds, whose id is a random, profile-local UUID.
  publisherId?: string
}

export interface OpmlData {
  items: OpmlItem[]
  channels: string[]
}

export interface ImportResult {
  subscribedPublishers: number
  addedDirectFeeds: number
  subscribedChannels: number
  // Items that were already subscribed, duplicates, or couldn't be resolved.
  skipped: number
}

// The side effects import performs, injected so the pure matching logic can be
// unit tested without a Mojo backend.
export interface ImportActions {
  // The full publisher catalog (all known publishers), used for matching.
  publishers: Publisher[]
  // All channels, used to validate channel names against the current locale.
  channels: Channel[]
  locale: string
  followPublisher: (publisherId: string) => void
  addDirectFeed: (url: string) => Promise<unknown>
  subscribeChannel: (locale: string, channelName: string) => void
}

// Serializes subscriptions into an OPML 2.0 document.
export function serializeOpml(data: OpmlData): string {
  const doc = document.implementation.createDocument(null, 'opml', null)
  const opml = doc.documentElement
  opml.setAttribute('version', '2.0')

  const head = doc.createElement('head')
  const title = doc.createElement('title')
  title.textContent = 'Brave News subscriptions'
  head.appendChild(title)
  opml.appendChild(head)

  const body = doc.createElement('body')
  opml.appendChild(body)

  for (const item of data.items) {
    const outline = doc.createElement('outline')
    outline.setAttribute('text', item.title)
    outline.setAttribute('title', item.title)
    if (item.xmlUrl) {
      outline.setAttribute('type', 'rss')
      outline.setAttribute('xmlUrl', item.xmlUrl)
    } else {
      outline.setAttribute('type', BRAVE_PUBLISHER_TYPE)
    }
    if (item.htmlUrl) {
      outline.setAttribute('htmlUrl', item.htmlUrl)
    }
    if (item.publisherId) {
      outline.setAttribute(BRAVE_PUBLISHER_ID_ATTR, item.publisherId)
    }
    body.appendChild(outline)
  }

  if (data.channels.length) {
    const group = doc.createElement('outline')
    group.setAttribute('text', 'Channels')
    group.setAttribute('title', 'Channels')
    for (const channelName of data.channels) {
      const outline = doc.createElement('outline')
      outline.setAttribute('text', channelName)
      outline.setAttribute('title', channelName)
      outline.setAttribute('type', BRAVE_CHANNEL_TYPE)
      group.appendChild(outline)
    }
    body.appendChild(group)
  }

  const xml = new XMLSerializer().serializeToString(doc)
  return `<?xml version="1.0" encoding="UTF-8"?>\n${xml}`
}

// The New Tab Page enforces Trusted Types, and its `default` policy has no
// `createHTML`, so DOMParser.parseFromString rejects a raw string. Route the
// OPML text through a dedicated policy (allow-listed in the New Tab Page's
// trusted-types CSP). The value is only ever parsed as XML and read for
// attribute values - it is never inserted into the live document as HTML.
let opmlTrustedTypesPolicy: { createHTML(input: string): string } | undefined
function toTrustedXml(xml: string): string {
  if (!opmlTrustedTypesPolicy) {
    // Note: This policy performs no sanitization, so the results from it should never be inserted
    // into the live document as HTML. It is only used for parsing OPML documents.
    opmlTrustedTypesPolicy = (window as any).trustedTypes.createPolicy('brave-news-opml', {
      createHTML: (input: string) => input
    })
  }
  return opmlTrustedTypesPolicy!.createHTML(xml)
}

// Parses an OPML document into feeds/publishers and channels. Throws if the
// document isn't valid XML.
export function parseOpml(xml: string): OpmlData {
  const doc = new DOMParser().parseFromString(toTrustedXml(xml), 'text/xml')
  if (doc.querySelector('parsererror')) {
    throw new Error('Failed to parse OPML document')
  }

  const items: OpmlItem[] = []
  const channels: string[] = []

  // A flat walk handles both top-level outlines and outlines nested under
  // category groups (e.g. the "Channels" group we emit, or category folders
  // produced by other readers).
  for (const outline of doc.querySelectorAll('outline')) {
    const type = outline.getAttribute('type')
    const title =
      outline.getAttribute('title') || outline.getAttribute('text') || ''

    if (type === BRAVE_CHANNEL_TYPE) {
      if (title) {
        channels.push(title)
      }
      continue
    }

    const xmlUrl = outline.getAttribute('xmlUrl') || undefined
    const publisherId =
      outline.getAttribute(BRAVE_PUBLISHER_ID_ATTR) || undefined

    // Only outlines that reference a feed or a Brave publisher are
    // subscriptions; category/grouping outlines are ignored.
    if (!xmlUrl && !publisherId) {
      continue
    }

    items.push({
      title,
      xmlUrl,
      htmlUrl: outline.getAttribute('htmlUrl') || undefined,
      publisherId
    })
  }

  return { items, channels }
}

// Maps a subscribed publisher to an OPML outline.
export function publisherToOpmlItem(publisher: Publisher): OpmlItem {
  return {
    title: publisher.publisherName,
    xmlUrl: publisher.feedSource?.url || undefined,
    htmlUrl: publisher.siteUrl?.url || undefined,
    // A direct feed's id is a random, per-profile UUID that is meaningless
    // elsewhere, so only combined publishers export their id.
    publisherId:
      publisher.type === PublisherType.COMBINED_SOURCE
        ? publisher.publisherId
        : undefined
  }
}

export function getSubscribedChannelNames(
  channels: Channel[],
  locale: string
): string[] {
  return channels
    .filter((c) => c.subscribedLocales.includes(locale))
    .map((c) => c.channelName)
}

// Builds the OPML payload from the current subscriptions.
export function buildExportData(
  publishers: Publisher[],
  channels: Channel[],
  locale: string
): OpmlData {
  return {
    items: publishers.filter(isPublisherEnabled).map(publisherToOpmlItem),
    channels: getSubscribedChannelNames(channels, locale)
  }
}

// Serializes the current subscriptions and triggers a file download.
export function downloadOpml(
  publishers: Publisher[],
  channels: Channel[],
  locale: string
) {
  const xml = serializeOpml(buildExportData(publishers, channels, locale))
  const blob = new Blob([xml], { type: 'text/x-opml' })
  const url = URL.createObjectURL(blob)

  const a = document.createElement('a')
  a.download = `brave-news-subscriptions-${new Date().toISOString()}.opml`
  a.href = url

  document.body.appendChild(a)
  a.click()
  a.remove()

  URL.revokeObjectURL(url)
}

// Normalizes a feed URL the same way the search box does (useSearch.ts), so our
// duplicate/publisher matching agrees with how feeds are added elsewhere.
function normalizeFeedUrl(raw: string): string | null {
  let url = raw.trim()
  if (!url) {
    return null
  }
  if (!url.includes('://')) {
    url = 'https://' + url
  }
  try {
    const parsed = new URL(url)
    if (!['http:', 'https:'].includes(parsed.protocol)) {
      return null
    }
  } catch {
    return null
  }
  return url
}

// Imports an OPML document, subscribing to matching publishers, adding new
// direct feeds (without duplicating existing ones), and re-subscribing channels.
export async function importOpml(
  xml: string,
  actions: ImportActions
): Promise<ImportResult> {
  const { items, channels } = parseOpml(xml)

  const result: ImportResult = {
    subscribedPublishers: 0,
    addedDirectFeeds: 0,
    subscribedChannels: 0,
    skipped: 0
  }

  const publishersById = new Map(
    actions.publishers.map((p) => [p.publisherId, p])
  )
  // Track existing direct feed URLs plus any we add during this import, so we
  // never create duplicates. The browser also dedupes by URL as a safety net.
  const directFeedUrls = new Set(
    actions.publishers
      .filter(isDirectFeed)
      .map((p) => p.feedSource?.url)
      .filter((u): u is string => !!u)
  )

  const subscribePublisher = (publisher: Publisher) => {
    if (isPublisherEnabled(publisher)) {
      result.skipped++
    } else {
      actions.followPublisher(publisher.publisherId)
      result.subscribedPublishers++
    }
  }

  const pending: Array<Promise<unknown>> = []

  for (const item of items) {
    // 1. Prefer an exact publisher id match. This is the only way to resolve a
    //    publisher that has no feed URL. An unknown id (e.g. a stale catalog)
    //    falls through to URL-based handling.
    if (item.publisherId) {
      const publisher = publishersById.get(item.publisherId)
      if (publisher) {
        subscribePublisher(publisher)
        continue
      }
    }

    const url = item.xmlUrl ? normalizeFeedUrl(item.xmlUrl) : null
    if (!url) {
      result.skipped++
      continue
    }

    // 2. Prefer a known publisher whose feed URL matches over creating a
    //    redundant direct feed (mirrors useSearch.ts).
    const publisherMatch = actions.publishers.find(
      (p) => p.feedSource?.url === url
    )
    if (publisherMatch) {
      subscribePublisher(publisherMatch)
      continue
    }

    // 3. Otherwise add a direct feed, skipping duplicates.
    if (directFeedUrls.has(url)) {
      result.skipped++
      continue
    }
    directFeedUrls.add(url)
    pending.push(Promise.resolve(actions.addDirectFeed(url)))
    result.addedDirectFeeds++
  }

  const channelByName = new Map(actions.channels.map((c) => [c.channelName, c]))
  for (const channelName of channels) {
    const channel = channelByName.get(channelName)
    // Channels are per-locale; only subscribe ones that exist in the current
    // locale and aren't already subscribed.
    if (!channel || channel.subscribedLocales.includes(actions.locale)) {
      result.skipped++
      continue
    }
    actions.subscribeChannel(actions.locale, channelName)
    result.subscribedChannels++
  }

  await Promise.all(pending)
  return result
}
