// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

extension WKContentWorld {
  static let cosmeticFiltersSandbox = WKContentWorld.world(name: "CosmeticFiltersSandbox")
}
