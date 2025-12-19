// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'

export default class BookmarksPageHandler
  implements Mojom.BookmarksPageHandlerInterface
{
  // Implement Closable<>
  public $ = { close() {} }

  getBookmarks() {
    return Promise.resolve({ bookmarks: [] })
  }
}
