// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import BraveShared
import Shared
import Data

private let log = Logger.browserLogger

protocol PlaylistFolderSharingHelperDelegate: AnyObject {
  func openPlaylistSharingFolder(with pageUrl: String)
}

class PlaylistFolderSharingHelper: NSObject, TabContentScript {
  fileprivate weak var tab: Tab?
  public weak var delegate: PlaylistFolderSharingHelperDelegate?

  init(tab: Tab) {
    self.tab = tab
    super.init()
  }

  static func name() -> String {
    return "PlaylistFolderSharingHelper"
  }

  func scriptMessageHandlerName() -> String? {
    return "playlistFolderSharingHelper_\(UserScriptManager.messageHandlerTokenString)"
  }

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    
    guard let body = message.body as? [String: Any],
          body["securitytoken"] as? String == UserScriptManager.securityTokenString else {
      return
    }
    
    if let sharingInfo = PlaylistFolderSharingInfo.from(message: message) {
      // This shared playlist folder already exists
      var sharedFolderPageUrl = sharingInfo.pageUrl
      if sharedFolderPageUrl.last != "/" {
        sharedFolderPageUrl += "/"
      }
      
      delegate?.openPlaylistSharingFolder(with: sharedFolderPageUrl)
    }
  }
}

private struct PlaylistFolderSharingInfo: Codable {
  public let pageUrl: String

  public static func from(message: WKScriptMessage) -> PlaylistFolderSharingInfo? {
    if !JSONSerialization.isValidJSONObject(message.body) {
      return nil
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body, options: [.fragmentsAllowed])
      return try JSONDecoder().decode(PlaylistFolderSharingInfo.self, from: data)
    } catch {
      log.error("Error Decoding PlaylistFolderSharingInfo: \(error)")
    }

    return nil
  }
}
