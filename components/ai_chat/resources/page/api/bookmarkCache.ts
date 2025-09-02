// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityCachingWrapper } from '$web-common/mojomCache'
import * as Mojom from '../../common/mojom'

/**
 * Maintains an up to date cache of the bookmarks available in the browser.
 */
export class BookmarkCachingWrapper extends EntityCachingWrapper<Mojom.Bookmark> implements Mojom.BookmarksListenerInterface {
  private service: Mojom.BookmarksServiceRemote
  private receiver = new Mojom.BookmarksListenerReceiver(this)
  constructor() {
    super()

    this.service = Mojom.BookmarksService.getRemote()
    this.service.addListener(this.receiver.$.bindNewPipeAndPassRemote())
  }
}
