// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension AdblockEngine {
  public func shouldBlock(requestUrl: String, requestHost: String, sourceHost: String) -> Bool {
    return matches(
      url: requestUrl,
      host: requestHost,
      tabHost: sourceHost,
      isThirdParty: requestUrl != sourceHost,
      resourceType: "script"
    ).didMatchRule
  }
  
  @available(*, deprecated, renamed: "deserialize(data:)")
  @discardableResult
  public func set(data: Data) -> Bool {
    deserialize(data: data)
  }
  
  @available(*, deprecated, message: "Use AdblockEngine.addResources(_:)")
  public func set(json: Data) -> Bool {
    guard let string = String(data: json, encoding: .utf8) else {
      return false
    }
    addResources(string)
    return true
  }
}

@available(*, deprecated, renamed: "AdblockEngine")
typealias AdblockRustEngine = AdblockEngine
