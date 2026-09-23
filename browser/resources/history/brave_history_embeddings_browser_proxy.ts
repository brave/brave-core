// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  PageCallbackRouter,
  PageHandlerFactory,
  PageHandlerRemote,
} from './brave_history_embeddings.mojom-webui.js'

export interface BraveHistoryEmbeddingsBrowserProxy {
  pageHandler: PageHandlerRemote
  callbackRouter: PageCallbackRouter
}

let instance: BraveHistoryEmbeddingsBrowserProxy | null = null

export function getBraveHistoryEmbeddingsBrowserProxy(): BraveHistoryEmbeddingsBrowserProxy {
  if (!instance) {
    const factory = PageHandlerFactory.getRemote()
    const pageHandler = new PageHandlerRemote()
    const callbackRouter = new PageCallbackRouter()
    factory.createInterfacePageHandler(
      callbackRouter.$.bindNewPipeAndPassRemote(),
      pageHandler.$.bindNewPipeAndPassReceiver(),
    )
    instance = { pageHandler, callbackRouter }
  }
  return instance
}
