// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import getBraveNewsController from './api'
import {
  BraveNewsControllerRemote,
  FeedListenerInterface,
  FeedListenerReceiver
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

export const addFeedListener = (listener: (feedHash: string) => void) =>
  new (class implements FeedListenerInterface {
    #receiver = new FeedListenerReceiver(this)
    #controller: BraveNewsControllerRemote

    constructor() {
      this.#controller = getBraveNewsController()

      if (process.env.NODE_ENV !== 'test') {
        this.#controller.addFeedListener(
          this.#receiver.$.bindNewPipeAndPassRemote()
        )
      }
    }

    onUpdateAvailable(feedHash: string): void {
      listener(feedHash)
    }
  })()
