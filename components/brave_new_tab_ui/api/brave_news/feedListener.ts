// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import getBraveNewsController from '.'
import {
  BraveNewsControllerRemote,
  FeedListenerInterface,
  FeedListenerReceiver
} from '../../../../../out/Component/gen/brave/components/brave_today/common/brave_news.mojom.m'

export const addFeedListener = (listener: (feedHash: string) => void) =>
  new (class implements FeedListenerInterface {
    #receiver = new FeedListenerReceiver(this)
    #controller: BraveNewsControllerRemote

    constructor () {
      this.#controller = getBraveNewsController()

      if (process.env.NODE_ENV !== 'test') {
        this.#controller.addFeedListener(
          this.#receiver.$.bindNewPipeAndPassRemote()
        )
      }
    }

    onUpdateAvailable (feedHash: string): void {
      listener(feedHash)
    }
  })()
