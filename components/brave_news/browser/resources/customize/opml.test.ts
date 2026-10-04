// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Channel, Publisher, PublisherType, UserEnabled } from '../shared/api'
import {
  buildExportData,
  importOpml,
  ImportActions,
  parseOpml,
  publisherToOpmlItem,
  serializeOpml
} from './opml'

const makePublisher = (overrides: Partial<Publisher>): Publisher =>
  ({
    publisherId: 'id',
    publisherName: 'Name',
    type: PublisherType.COMBINED_SOURCE,
    categoryName: '',
    isEnabled: true,
    locales: [],
    feedSource: { url: '' },
    faviconUrl: undefined,
    coverUrl: undefined,
    backgroundColor: undefined,
    siteUrl: { url: '' },
    userEnabledStatus: UserEnabled.NOT_MODIFIED,
    ...overrides
  }) as Publisher

const makeChannel = (overrides: Partial<Channel>): Channel =>
  ({
    channelName: 'Channel',
    subscribedLocales: [],
    ...overrides
  }) as Channel

const makeActions = (overrides: Partial<ImportActions>): ImportActions => ({
  publishers: [],
  channels: [],
  locale: 'en_US',
  followPublisher: jest.fn(),
  addDirectFeed: jest.fn(async () => {}),
  subscribeChannel: jest.fn(),
  ...overrides
})

describe('serialize/parse round-trip', () => {
  it('round-trips feeds, id-only publishers, direct feeds and channels', () => {
    const data = {
      items: [
        {
          title: 'Combined',
          xmlUrl: 'https://combined.example/feed',
          htmlUrl: 'https://combined.example',
          publisherId: 'pub-1'
        },
        { title: 'No Feed', htmlUrl: 'https://nofeed.example', publisherId: 'pub-2' },
        { title: 'My Blog', xmlUrl: 'https://blog.example/feed' }
      ],
      channels: ['Technology', 'Sports']
    }

    const parsed = parseOpml(serializeOpml(data))
    expect(parsed).toEqual(data)
  })

  it('produces a well-formed OPML document', () => {
    const xml = serializeOpml({
      items: [{ title: 'X', xmlUrl: 'https://x.example/feed' }],
      channels: []
    })
    expect(xml).toContain('<?xml')
    expect(xml).toContain('<opml version="2.0"')
    expect(xml).toContain('type="rss"')
    expect(xml).toContain('xmlUrl="https://x.example/feed"')
  })
})

describe('parseOpml', () => {
  it('finds feeds nested inside category outlines', () => {
    const xml = `<?xml version="1.0"?>
      <opml version="2.0"><body>
        <outline text="Tech">
          <outline type="rss" text="A" xmlUrl="https://a.example/feed"/>
        </outline>
      </body></opml>`
    const { items } = parseOpml(xml)
    expect(items).toEqual([
      { title: 'A', xmlUrl: 'https://a.example/feed', htmlUrl: undefined, publisherId: undefined }
    ])
  })

  it('falls back to text when title is missing', () => {
    const xml = `<opml><body>
      <outline type="rss" text="Only Text" xmlUrl="https://t.example/feed"/>
    </body></opml>`
    expect(parseOpml(xml).items[0].title).toBe('Only Text')
  })

  it('ignores grouping outlines without a feed url or publisher id', () => {
    const xml = `<opml><body>
      <outline text="Just a folder"/>
      <outline type="rss" text="Feed" xmlUrl="https://f.example/feed"/>
    </body></opml>`
    expect(parseOpml(xml).items).toHaveLength(1)
  })

  it('collects brave-channel outlines as channels', () => {
    const xml = `<opml><body>
      <outline text="Channels">
        <outline type="brave-channel" text="Gaming"/>
      </outline>
    </body></opml>`
    const { items, channels } = parseOpml(xml)
    expect(items).toHaveLength(0)
    expect(channels).toEqual(['Gaming'])
  })

  it('throws on malformed XML', () => {
    expect(() => parseOpml('<opml><body></not-closed>')).toThrow()
  })
})

describe('buildExportData', () => {
  it('includes an id for combined publishers but not direct feeds', () => {
    const combined = makePublisher({
      publisherId: 'pub-1',
      publisherName: 'Combined',
      feedSource: { url: 'https://c.example/feed' },
      siteUrl: { url: 'https://c.example' },
      userEnabledStatus: UserEnabled.ENABLED
    })
    const direct = makePublisher({
      publisherId: 'uuid-123',
      publisherName: 'Direct',
      type: PublisherType.DIRECT_SOURCE,
      feedSource: { url: 'https://d.example/feed' }
    })

    expect(publisherToOpmlItem(combined)).toEqual({
      title: 'Combined',
      xmlUrl: 'https://c.example/feed',
      htmlUrl: 'https://c.example',
      publisherId: 'pub-1'
    })
    expect(publisherToOpmlItem(direct).publisherId).toBeUndefined()
  })

  it('exports a combined publisher with no feed url by id only', () => {
    const item = publisherToOpmlItem(
      makePublisher({
        publisherId: 'pub-2',
        feedSource: { url: '' },
        userEnabledStatus: UserEnabled.ENABLED
      })
    )
    expect(item.xmlUrl).toBeUndefined()
    expect(item.publisherId).toBe('pub-2')
  })

  it('only exports subscribed publishers and channels for the locale', () => {
    const subscribed = makePublisher({
      publisherId: 'sub',
      userEnabledStatus: UserEnabled.ENABLED,
      feedSource: { url: 'https://sub.example/feed' }
    })
    const notSubscribed = makePublisher({ publisherId: 'no' })
    const channels = [
      makeChannel({ channelName: 'Tech', subscribedLocales: ['en_US'] }),
      makeChannel({ channelName: 'Other', subscribedLocales: ['fr_FR'] })
    ]

    const data = buildExportData([subscribed, notSubscribed], channels, 'en_US')
    expect(data.items.map((i) => i.publisherId)).toEqual(['sub'])
    expect(data.channels).toEqual(['Tech'])
  })
})

describe('importOpml', () => {
  it('subscribes a publisher by id (even with no feed url) and adds no direct feed', async () => {
    const publisher = makePublisher({
      publisherId: 'pub-1',
      feedSource: { url: '' }
    })
    const actions = makeActions({ publishers: [publisher] })
    const xml = serializeOpml({
      items: [{ title: 'No Feed', publisherId: 'pub-1' }],
      channels: []
    })

    const result = await importOpml(xml, actions)
    expect(actions.followPublisher).toHaveBeenCalledWith('pub-1')
    expect(actions.addDirectFeed).not.toHaveBeenCalled()
    expect(result.subscribedPublishers).toBe(1)
    expect(result.addedDirectFeeds).toBe(0)
  })

  it('prefers a publisher matched by feed url over creating a direct feed', async () => {
    const publisher = makePublisher({
      publisherId: 'pub-1',
      feedSource: { url: 'https://match.example/feed' }
    })
    const actions = makeActions({ publishers: [publisher] })
    // No publisher id in the outline - must match by URL.
    const xml = `<opml><body>
      <outline type="rss" text="Match" xmlUrl="https://match.example/feed"/>
    </body></opml>`

    const result = await importOpml(xml, actions)
    expect(actions.followPublisher).toHaveBeenCalledWith('pub-1')
    expect(actions.addDirectFeed).not.toHaveBeenCalled()
    expect(result.subscribedPublishers).toBe(1)
  })

  it('skips a url that is already a direct feed', async () => {
    const existingDirect = makePublisher({
      publisherId: 'uuid-1',
      type: PublisherType.DIRECT_SOURCE,
      feedSource: { url: 'https://blog.example/feed' }
    })
    const actions = makeActions({ publishers: [existingDirect] })
    const xml = `<opml><body>
      <outline type="rss" text="Dup" xmlUrl="https://blog.example/feed"/>
    </body></opml>`

    const result = await importOpml(xml, actions)
    expect(actions.addDirectFeed).not.toHaveBeenCalled()
    expect(result.addedDirectFeeds).toBe(0)
    expect(result.skipped).toBe(1)
  })

  it('adds a brand new url as a direct feed', async () => {
    const actions = makeActions({})
    const xml = `<opml><body>
      <outline type="rss" text="New" xmlUrl="https://new.example/feed"/>
    </body></opml>`

    const result = await importOpml(xml, actions)
    expect(actions.addDirectFeed).toHaveBeenCalledWith('https://new.example/feed')
    expect(result.addedDirectFeeds).toBe(1)
  })

  it('does not duplicate direct feeds within the same import', async () => {
    const actions = makeActions({})
    const xml = `<opml><body>
      <outline type="rss" text="A" xmlUrl="https://dup.example/feed"/>
      <outline type="rss" text="B" xmlUrl="https://dup.example/feed"/>
    </body></opml>`

    const result = await importOpml(xml, actions)
    expect(actions.addDirectFeed).toHaveBeenCalledTimes(1)
    expect(result.addedDirectFeeds).toBe(1)
    expect(result.skipped).toBe(1)
  })

  it('skips an already-subscribed publisher', async () => {
    const publisher = makePublisher({
      publisherId: 'pub-1',
      feedSource: { url: 'https://sub.example/feed' },
      userEnabledStatus: UserEnabled.ENABLED
    })
    const actions = makeActions({ publishers: [publisher] })
    const xml = serializeOpml({
      items: [{ title: 'Sub', xmlUrl: 'https://sub.example/feed', publisherId: 'pub-1' }],
      channels: []
    })

    const result = await importOpml(xml, actions)
    expect(actions.followPublisher).not.toHaveBeenCalled()
    expect(result.skipped).toBe(1)
    expect(result.subscribedPublishers).toBe(0)
  })

  it('falls back to url handling when the publisher id is unknown', async () => {
    const actions = makeActions({})
    const xml = serializeOpml({
      items: [
        { title: 'Stale', xmlUrl: 'https://stale.example/feed', publisherId: 'gone' }
      ],
      channels: []
    })

    const result = await importOpml(xml, actions)
    expect(actions.addDirectFeed).toHaveBeenCalledWith('https://stale.example/feed')
    expect(result.addedDirectFeeds).toBe(1)
  })

  it('re-subscribes channels for the current locale', async () => {
    const channels = [
      makeChannel({ channelName: 'Tech', subscribedLocales: [] }),
      makeChannel({ channelName: 'Already', subscribedLocales: ['en_US'] })
    ]
    const actions = makeActions({ channels, locale: 'en_US' })
    const xml = `<opml><body>
      <outline text="Channels">
        <outline type="brave-channel" text="Tech"/>
        <outline type="brave-channel" text="Already"/>
        <outline type="brave-channel" text="Unknown"/>
      </outline>
    </body></opml>`

    const result = await importOpml(xml, actions)
    expect(actions.subscribeChannel).toHaveBeenCalledTimes(1)
    expect(actions.subscribeChannel).toHaveBeenCalledWith('en_US', 'Tech')
    expect(result.subscribedChannels).toBe(1)
    // 'Already' (subscribed) and 'Unknown' (not in locale) are skipped.
    expect(result.skipped).toBe(2)
  })

  it('rejects on malformed OPML', async () => {
    await expect(importOpml('<opml></body>', makeActions({}))).rejects.toThrow()
  })
})
