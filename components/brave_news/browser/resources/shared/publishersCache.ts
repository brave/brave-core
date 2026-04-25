// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveNewsControllerRemote,
  Publisher,
  PublishersListenerInterface,
  PublishersListenerReceiver,
  UserEnabled
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import getBraveNewsController, { isDirectFeed } from './api'

import { EntityCachingWrapper } from '$web-common/mojomCache'

let instance: PublishersCachingWrapper | null = null

export class PublishersCachingWrapper
  extends EntityCachingWrapper<Publisher>
  implements PublishersListenerInterface {
  private receiver = new PublishersListenerReceiver(this)
  private controller: BraveNewsControllerRemote

  constructor() {
    super()

    this.controller = getBraveNewsController()

    // We can't set up  the mojo pipe in the test environment.
    if (process.env.NODE_ENV !== 'test') {
      this.controller.addPublishersListener(
        this.receiver.$.bindNewPipeAndPassRemote()
      )
    }
  }

  static getInstance() {
    if (!instance) {
      instance = new this()
    }
    return instance
  }

  setPublisherFollowed(publisherId: string, enabled: boolean) {
    const copy = {
      ...this.cache
    }

    if (isDirectFeed(this.cache[publisherId]) && !enabled) {
      this.controller.setPublisherPref(publisherId, UserEnabled.DISABLED)
      delete copy[publisherId]
    } else {
      const status = enabled ? UserEnabled.ENABLED : UserEnabled.NOT_MODIFIED
      this.controller.setPublisherPref(publisherId, status)
      copy[publisherId].userEnabledStatus = status
    }

    this.notifyChanged(copy)
  }
}
