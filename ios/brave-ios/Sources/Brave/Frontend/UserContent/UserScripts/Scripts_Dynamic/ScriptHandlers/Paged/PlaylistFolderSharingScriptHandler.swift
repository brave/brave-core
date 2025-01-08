// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
import Foundation
import Shared
import WebKit
import os.log

protocol PlaylistFolderSharingScriptHandlerDelegate: AnyObject {
  func openPlaylistSharingFolder(with pageUrl: String)
}

class PlaylistFolderSharingScriptHandler: NSObject, TabContentScript {
  public weak var delegate: PlaylistFolderSharingScriptHandlerDelegate?

  static let scriptName = "PlaylistFolderSharingScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
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
      let data = try JSONSerialization.data(
        withJSONObject: message.body,
        options: [.fragmentsAllowed]
      )
      return try JSONDecoder().decode(PlaylistFolderSharingInfo.self, from: data)
    } catch {
      Logger.module.error("Error Decoding PlaylistFolderSharingInfo: \(error.localizedDescription)")
    }

    return nil
  }
}
