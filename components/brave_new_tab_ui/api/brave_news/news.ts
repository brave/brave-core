// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveNewsControllerRemote, Publisher, PublisherType, UserEnabled } from 'gen/brave/components/brave_today/common/brave_news.mojom.m'
import getBraveNewsController, { Channels } from '.'

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

  channelsListeners: ChannelsListener[] = []
  lastChannels: Channels = {}

  locale: string

  constructor () {
    this.controller = getBraveNewsController()
  }

  update () {
    this.updateChannels()

    this.controller.getLocale().then(({ locale }) => {
      this.locale = locale
    })
  }

  getChannels () {
    return this.lastChannels
  }

  async setChannelSubscribed (channelId: string, subscribed: boolean) {
    // While we're waiting for the new channels to come back, speculatively
    // update them, so the UI has instant feedback.
    // This will be overwritten when the controller responds.
    let subscribedLocales = this.lastChannels[channelId]?.subscribedLocales ?? []
    if (subscribedLocales.includes(this.locale)) {
      // Remove this locale from the list of subscribed locales.
      subscribedLocales = subscribedLocales.filter(l => l !== this.locale)
    } else {
      // Add this locale to the list of subscribed locales.
      subscribedLocales.push(this.locale)
    }

    this.updateChannels({
      ...this.lastChannels,
      [channelId]: {
        ...this.lastChannels[channelId],
        subscribedLocales
      }
    })

    // Then, once we receive the actual update, apply it.
    const { updated } = await this.controller.setChannelSubscribed(this.locale, channelId, subscribed)
    this.updateChannels({
      ...this.lastChannels,
      [channelId]: updated
    })
  }

  async updateChannels (newChannels?: Channels) {
    if (!newChannels) {
      ({ channels: newChannels } = await this.controller.getChannels())
    }

    const oldValue = this.lastChannels
    this.lastChannels = newChannels!

    this.notifyChannelsListeners(this.lastChannels, oldValue)
  }

  addChannelsListener (listener: ChannelsListener) {
    this.channelsListeners.push(listener)
  }

  removeChannelsListener (listener: ChannelsListener) {
    const index = this.channelsListeners.indexOf(listener)
    this.channelsListeners.splice(index, 1)
  }

  notifyChannelsListeners (newValue: Channels, oldValue: Channels) {
    for (const listener of this.channelsListeners) {
      listener(newValue, oldValue)
    }
  }
}

export const api = new BraveNewsApi();
(window as any).api = api
