// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

#if DEBUG

class TestIpfsAPI: IpfsAPI {
  var ipfsGateway: URL?
  var nftIpfsGateway: URL?

  var _resolveGatewayUrl: ((_ input: URL) -> URL?)?
  func resolveGatewayUrl(for input: URL) -> URL? {
    _resolveGatewayUrl?(input)
  }

  var _resolveGatewayUrlForNft: ((_ input: URL) -> URL?)?
  func resolveGatewayUrl(forNft input: URL) -> URL? {
    _resolveGatewayUrlForNft?(input)
  }

  var _contentHashToCIDv1URL: ((_ contentHash: [NSNumber]) -> URL?)?
  func contentHashToCIDv1URL(for contentHash: [NSNumber]) -> URL? {
    _contentHashToCIDv1URL?(contentHash)
  }
}

#endif
