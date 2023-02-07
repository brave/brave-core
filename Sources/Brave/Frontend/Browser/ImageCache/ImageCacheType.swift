// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

public enum ImageCacheType: Int, CustomDebugStringConvertible {

  /// The image was not available in the cache, but was downloaded from the web.
  case none

  /// The image was obtained from the memory cache.
  case memory

  /// The image was obtained from the disk cache.
  case disk

  public var debugDescription: String {
    switch self {
    case .none:
      return "Image was not available in the cache, but was downloaded from the web"

    case .memory:
      return "Image was obtained from the memory cache"

    case .disk:
      return "Image was obtained from the disk cache"
    }
  }

}
