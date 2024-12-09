// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveNewsControllerRemote,
  Publisher,
  ListenerInterface,
  ListenerReceiver,
  UserEnabled
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import getBraveNewsController, { isDirectFeed } from './api'

import { CachingWrapper } from '$web-common/mojomCache'
import { Value } from 'gen/mojo/public/mojom/base/values.mojom.m'

export class PublishersCachingWrapper
  extends CachingWrapper<{ [key: string]: Publisher }>
  implements ListenerInterface {
  private receiver = new ListenerReceiver(this)
  private controller: BraveNewsControllerRemote

  constructor() {
    super({})

    this.controller = getBraveNewsController()

    // We can't set up  the mojo pipe in the test environment.
    if (process.env.NODE_ENV !== 'test') {
      this.controller.addListener(
        this.receiver.$.bindNewPipeAndPassRemote()
      )
    }
  }

  changed(diff: Value): void {
    if (!diff) return

    
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
