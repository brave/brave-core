// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveNewsControllerRemote,
  Channel,
  ChannelsListenerInterface,
  ChannelsListenerReceiver
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import getBraveNewsController from './api'
import { EntityCachingWrapper } from '$web-common/mojomCache'

export class ChannelsCachingWrapper
  extends EntityCachingWrapper<Channel>
  implements ChannelsListenerInterface {
  private receiver = new ChannelsListenerReceiver(this)
  private controller: BraveNewsControllerRemote

  constructor() {
    super()

    this.controller = getBraveNewsController()

    // We can't setup the mojom pipe in the test environment.
    if (process.env.NODE_ENV !== 'test') {
      this.controller.addChannelsListener(
        this.receiver.$.bindNewPipeAndPassRemote()
      )
    }
  }

  setChannelSubscribed(locale: string, channelId: string, subscribed: boolean) {
    // While we're waiting for the new channels to come back, speculatively
    // update them, so the UI has instant feedback.
    // This will be overwritten when the controller responds.
    let subscribedLocales = [
      ...(this.cache[channelId]?.subscribedLocales ?? [])
    ]
    if (subscribedLocales.includes(locale)) {
      // Remove this locale from the list of subscribed locales.
      subscribedLocales = subscribedLocales.filter((l) => l !== locale)
    } else {
      // Add this locale to the list of subscribed locales.
      subscribedLocales.push(locale)
    }

    this.change({
      ...this.cache,
      [channelId]: {
        ...this.cache[channelId],
        subscribedLocales
      }
    })

    return this.controller.setChannelSubscribed(locale, channelId, subscribed)
  }
}
