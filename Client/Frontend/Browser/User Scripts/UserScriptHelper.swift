// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Data
import BraveShared
import BraveCore
import Shared

/// A class that helps in handling user scripts.
class UserScriptHelper {
  /// Object that represent the settings when returning deomian script types.
  struct DomainScriptOptions: OptionSet {
    let rawValue: Int

    /// Wether or not we should persist shield settings
    static let persistShieldSettings = DomainScriptOptions(rawValue: 1 << 0)

    /// These settings are tuned for a regular browsing session:
    /// - Persist shield settings
    static let `default`: DomainScriptOptions = [persistShieldSettings]

    /// These settings are tuned for a private browsing session:
    /// - Does not persist shield settings
    static let privateBrowsing: DomainScriptOptions = []

    /// These options are tuned for the `PlaylistCacheLoader`:
    /// - Does not persist shield settings
    static let playlistCacheLoader: DomainScriptOptions = []
  }

  /// Return the set of DomainScryptTypes for this navigation action.
  static func getUserScriptTypes(for navigationAction: WKNavigationAction, options: DomainScriptOptions) -> Set<UserScriptType> {
    var userScriptTypes: Set<UserScriptType> = []

    // Handle dynamic domain level scripts on the request that don't use shields
    // This shield is always on and doesn't need sheild settings
    if let url = navigationAction.request.url, let domainUserScript = DomainUserScript(for: url), domainUserScript.shieldType == nil {
      userScriptTypes.insert(.domainUserScript(domainUserScript))
    }

    // Handle dynamic domain level scripts on the main document.
    // These are scripts that change depending on the domain and the main document
    if let mainDocumentURL = navigationAction.request.mainDocumentURL {
      let domainForShields = Domain.getOrCreate(forUrl: mainDocumentURL, persistent: options.contains(.persistShieldSettings))
      let isFPProtectionOn = domainForShields.isShieldExpected(.FpProtection, considerAllShieldsOption: true)

      // Add the `farblingProtection` script if needed
      // Note: The added farbling protection script based on the document url, not the frame's url.
      // It is also added for every frame, including subframes.
      if let etldP1 = mainDocumentURL.baseDomain, isFPProtectionOn {
        userScriptTypes.insert(.nacl) // dependency for `farblingProtection`
        userScriptTypes.insert(.farblingProtection(etld: etldP1))
      }
      
      // Handle dynamic domain level scripts on the request that do use shields (youtube)
      if let url = navigationAction.request.url, let domainUserScript = DomainUserScript(for: url), let shieldType = domainUserScript.shieldType {
        if domainForShields.isShieldExpected(shieldType, considerAllShieldsOption: true) {
          // Add the shield only if its enabled
          userScriptTypes.insert(.domainUserScript(domainUserScript))
        }
      }
    }

    return userScriptTypes
  }
  
  static func makeDeAmpScriptParamters() throws -> String? {
    let arguments: [String: String] = [
      "handlerName": DeAmpHelper.scriptMessageHandlerName(),
      "securityToken": UserScriptManager.securityTokenString
    ]
    
    let encoder = JSONEncoder()
    let data = try encoder.encode(arguments)
    return String(data: data, encoding: .utf8)
  }

  static func makeRequestBlockingParams(securityToken: String) throws -> String? {
    let params: [String: String] = [
      "securityToken": securityToken,
      "handlerName": RequestBlockingContentHelper.scriptMessageHandlerName()
    ]
    let encoder = JSONEncoder()
    let data = try encoder.encode(params)
    return String(data: data, encoding: .utf8)
  }
}
