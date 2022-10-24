// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveNewsControllerRemote, Publisher, PublisherType, UserEnabled } from 'gen/brave/components/brave_today/common/brave_news.mojom.m'
import getBraveNewsController, { Channels, Publishers } from '.'

type PublishersListener = (publishers: Publishers, oldValue: Publishers) => void
type ChannelsListener = (newValue: Channels, oldValue: Channels) => void

export const isPublisherEnabled = (publisher: Publisher) => {
  if (!publisher) return false

  // Direct Sources are enabled if they're available.
  if (publisher.type === PublisherType.DIRECT_SOURCE) return true

  // Publishers enabled via channel are not shown in the sidebar.
  return publisher.userEnabledStatus === UserEnabled.ENABLED
}

export const isDirectFeed = (publisher: Publisher) => {
  if (!publisher) return false
  return publisher.type === PublisherType.DIRECT_SOURCE
}

class BraveNewsApi {
  controller: BraveNewsControllerRemote

  publishersListeners: PublishersListener[] = []
  lastPublishers: Publishers = {}

  channelsListeners: ChannelsListener[] = []
  lastChannels: Channels = {}

  locale: string

  constructor () {
    this.controller = getBraveNewsController()
    this.updateChannels()

    this.controller.getLocale().then(({ locale }) => {
      this.locale = locale
      this.updatePublishers()
    })
  }

  getPublishers () {
    return this.lastPublishers
  }

  getChannels () {
    return this.lastChannels
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
    const publisher = this.lastPublishers[publisherId]
    if (isDirectFeed(publisher) && status === UserEnabled.DISABLED) {
      delete newValue[publisherId]
    }

    this.controller.setPublisherPref(publisherId, status)
    this.updatePublishers(newValue)
  }

  async subscribeToDirectFeed (feedUrl: string) {
    const { publishers } = await this.controller.subscribeToNewDirectFeed({ url: feedUrl })
    this.updatePublishers(publishers)
  }

  setPublisherFollowed (publisherId: string, enabled: boolean) {
    // For now, Direct Sources work differently to Combine Sources - in their
    // not modified state they are considered enabled.
    if (this.lastPublishers[publisherId]?.type === PublisherType.DIRECT_SOURCE && !enabled) {
      this.setPublisherPref(publisherId, UserEnabled.DISABLED)
      return
    }

    this.setPublisherPref(publisherId, enabled ? UserEnabled.ENABLED : UserEnabled.NOT_MODIFIED)
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
    if (!newPublishers) {
      ({ publishers: newPublishers } = await this.controller.getPublishers())
    }

    const oldValue = this.lastPublishers
    this.lastPublishers = newPublishers!

    this.notifyPublishersListeners(newPublishers!, oldValue)
  }

  async updateChannels (newChannels?: Channels) {
    if (!newChannels) {
      ({ channels: newChannels } = await this.controller.getChannels())
    }

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

  notifyPublishersListeners (newValue: Publishers, oldValue: Publishers) {
    for (const listener of this.publishersListeners) {
      listener(newValue, oldValue)
    }
  }

  notifyChannelsListeners (newValue: Channels, oldValue: Channels) {
    for (const listener of this.channelsListeners) {
      listener(newValue, oldValue)
    }
  }
}

export const api = new BraveNewsApi()
