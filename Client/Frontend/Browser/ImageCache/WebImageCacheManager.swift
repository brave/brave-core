// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

final public class WebImageCacheManager {

  private let webImageCache: WebImageCache

  public static let shared: WebImageCache = {
    let webImageCacheManager = WebImageCacheManager(isPrivate: true)
    return webImageCacheManager.webImageCache
  }()

  private init(isPrivate: Bool) {
    let webImageCache = WebImageCache(isPrivate: isPrivate)
    self.webImageCache = webImageCache
  }

}
