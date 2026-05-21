// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// A scheme handler to deal with the old NTP urls while users migrate away from them
public class LegacyNTPHandler: InternalSchemeResponse {
  public static let path = "about/home"
  public func response(forRequest request: URLRequest) async -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    return (InternalSchemeHandler.response(forUrl: url), Data())
  }
  public init() {}
}
