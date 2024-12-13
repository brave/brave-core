// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveNewsControllerRemote,
  Configuration,
  ListenerInterface,
  ListenerReceiver,
  State
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import getBraveNewsController from './api'

import { CachingWrapper, valueToJS } from '$web-common/mojomCache'
import { Value } from 'gen/mojo/public/mojom/base/values.mojom.m'

export class ConfigurationCachingWrapper
  extends CachingWrapper<Configuration>
  implements ListenerInterface {
  private receiver = new ListenerReceiver(this)
  private controller: BraveNewsControllerRemote

  constructor() {
    super({
      isOptedIn: false,
      showOnNTP: false,
      openArticlesInNewTab: true,
    })

    this.controller = getBraveNewsController()

    // We can't set up  the mojo pipe in the test environment.
    if (process.env.NODE_ENV !== 'test') {
      this.controller.addListener(
        this.receiver.$.bindNewPipeAndPassRemote()
      )
    }
  }

  get value() {
    return this.cache;
  }

  set(change: Partial<Configuration>) {
    const newValue = {...this.cache, ...change }
    this.controller.setConfiguration(newValue)
    this.notifyChanged(newValue)
  }

  changed(diff: Value): void {
    const parsed = valueToJS<Partial<State>>(diff).configuration
    console.log('diff', parsed)
    if (!diff) {
      return
    }

    this.notifyChanged({
      ...this.cache,
      ...parsed,
    })
  }
}
