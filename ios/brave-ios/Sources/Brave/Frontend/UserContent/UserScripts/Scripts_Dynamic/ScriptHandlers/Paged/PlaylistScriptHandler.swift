// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import Data
import Foundation
import Playlist
import Preferences
import Shared
import Web
import WebKit
import os.log

enum PlaylistItemAddedState {
  case none
  case newItem
  case existingItem
}

protocol PlaylistScriptHandlerDelegate: NSObject {
  func updatePlaylistURLBar(
    tab: (any TabState)?,
    state: PlaylistItemAddedState,
    item: PlaylistInfo?
  )
  func showPlaylistPopover(tab: (any TabState)?)
  func showPlaylistToast(tab: (any TabState)?, state: PlaylistItemAddedState, item: PlaylistInfo?)
  func showPlaylistAlert(tab: (any TabState)?, state: PlaylistItemAddedState, item: PlaylistInfo?)
  func showPlaylistOnboarding(tab: (any TabState)?)
}

class PlaylistScriptHandler: NSObject, TabContentScript, TabObserver {
  public weak var delegate: PlaylistScriptHandlerDelegate?
  private var url: URL?
  private var asset: AVURLAsset?
  private static let queue = DispatchQueue(label: "com.playlisthelper.queue", qos: .userInitiated)

  init(tab: some TabState) {
    self.url = tab.visibleURL
    super.init()

    tab.addObserver(self)
  }

  deinit {
    asset?.cancelLoading()
  }

  static let playlistLongPressed = "playlistLongPressed_\(uniqueID)"
  static let playlistProcessDocumentLoad = "playlistProcessDocumentLoad_\(uniqueID)"
  static let mediaCurrentTimeFromTag = "mediaCurrentTimeFromTag_\(uniqueID)"
  static let stopMediaPlayback = "stopMediaPlayback_\(uniqueID)"

  static let scriptName = "PlaylistScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerNamesMap: [
          "$<message_handler>": messageHandlerName,
          "$<tagUUID>": "tagId_\(uniqueID)",
          "$<sendMessageTimeout>": "smt_\(uniqueID)",
          "$<playlistLongPressed>": playlistLongPressed,
          "$<playlistProcessDocumentLoad>": playlistProcessDocumentLoad,
          "$<mediaCurrentTimeFromTag>": mediaCurrentTimeFromTag,
          "$<stopMediaPlayback>": stopMediaPlayback,
        ],
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    // If this URL is blocked from Playlist support, do nothing
    if url?.isPlaylistBlockedSiteURL == true {
      return
    }

    if ReadyState.from(message: message) != nil {
      return
    }

    Self.processPlaylistInfo(
      tab: tab,
      handler: self,
      item: PlaylistInfo.from(message: message)
    )
  }

  private class func processPlaylistInfo(
    tab: some TabState,
    handler: PlaylistScriptHandler,
    item: PlaylistInfo?
  ) {
    guard var item = item, !item.src.isEmpty else {
      DispatchQueue.main.async {
        handler.delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
      }
      return
    }

    if handler.url?.baseDomain != "soundcloud.com", item.isInvisible {
      DispatchQueue.main.async {
        handler.delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
      }
      return
    }

    // Copy the item but use the web-view's title and location instead, if available
    // This is due to a iFrames security
    item.pageSrc = tab.visibleURL?.absoluteString ?? item.pageSrc
    item.pageTitle = tab.title ?? item.pageTitle
    item.lastPlayedOffset = 0.0

    Self.queue.async { [weak handler] in
      guard let handler = handler else { return }

      if item.duration <= 0.0 && !item.detected || item.src.isEmpty {
        DispatchQueue.main.async {
          handler.delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
        }
        return
      }

      if URL(string: item.src) != nil {
        DispatchQueue.main.async { [weak handler] in
          guard let handler = handler,
            let delegate = handler.delegate
          else { return }

          if PlaylistItem.itemExists(pageSrc: item.pageSrc) {
            // Item already exists, so just update the database with new token or URL.
            handler.updateItem(item, detected: item.detected, tab: tab)
          } else if item.detected {
            // Automatic Detection
            delegate.updatePlaylistURLBar(tab: tab, state: .newItem, item: item)
            delegate.showPlaylistOnboarding(tab: tab)
          } else {
            // Long-Press
            delegate.showPlaylistAlert(tab: tab, state: .newItem, item: item)
          }
        }
      }
    }
  }

  private func loadAssetPlayability(url: URL) async -> Bool {
    if asset == nil {
      // We have to create an AVURLAsset here to determine if the item is playable
      // because otherwise it will add an invalid item to playlist that can't be played.
      // IE: WebM videos aren't supported so can't be played.
      // Therefore we shouldn't prompt the user to add to playlist.
      asset = AVURLAsset(url: url, options: AVAsset.defaultOptions)
    }

    guard let asset = asset else {
      return false
    }

    return await PlaylistMediaStreamer.loadAssetPlayability(asset: asset)
  }

  private func updateItem(_ item: PlaylistInfo, detected: Bool, tab: some TabState) {
    if detected {
      self.delegate?.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
    }

    PlaylistItem.updateItem(item) { [weak self] in
      guard let self = self else { return }

      // We need to use the Database version of this object
      // because when the fallback streamer updates the object, it uses the database ID.
      // When the download starts, it uses the database ID.
      // If we suddenly change the ID, downloads and updates get out of wack
      var item = item

      // Use the ID that it was saved as in the database, rather than the Javascript ID
      item.tagId = $0

      if let delegate = self.delegate {
        if detected {
          delegate.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
        } else {
          delegate.showPlaylistToast(tab: tab, state: .existingItem, item: item)
        }
      }
    }
  }

  // MARK: - TabObserver

  func tabDidUpdateURL(_ tab: some TabState) {
    url = tab.visibleURL
    asset?.cancelLoading()
    asset = nil

    delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

extension PlaylistScriptHandler {
  static func getCurrentTime(
    tab: some TabState,
    nodeTag: String,
    completion: @escaping (Double) -> Void
  ) {
    guard UUID(uuidString: nodeTag) != nil else {
      Logger.module.error("Unsanitized NodeTag.")
      return
    }

    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(mediaCurrentTimeFromTag)",
      args: [nodeTag, Self.scriptId],
      contentWorld: Self.scriptSandbox,
      asFunction: true
    ) { value, error in

      if let error = error {
        Logger.module.error(
          "Error Retrieving Playlist Page Media Current Time: \(error.localizedDescription)"
        )
      }

      DispatchQueue.main.async {
        if let value = value as? Double {
          completion(value)
        } else {
          completion(0.0)
        }
      }
    }
  }

  static func stopPlayback(tab: (any TabState)?) {
    guard let tab = tab else { return }

    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(stopMediaPlayback)",
      args: [Self.scriptId],
      contentWorld: Self.scriptSandbox,
      asFunction: true
    ) { value, error in
      if let error = error {
        Logger.module.error(
          "Error Retrieving Stopping Media Playback: \(error.localizedDescription)"
        )
      }
    }
  }
}

extension PlaylistScriptHandler {
  static func updatePlaylistTab(tab: some TabState, item: PlaylistInfo?) {
    if let handler = tab.browserData?.getContentScript(name: Self.scriptName)
      as? PlaylistScriptHandler
    {
      Self.processPlaylistInfo(tab: tab, handler: handler, item: item)
    }
  }
}

extension PlaylistScriptHandler {
  struct ReadyState: Codable {
    let state: String

    static func from(message: WKScriptMessage) -> ReadyState? {
      if !JSONSerialization.isValidJSONObject(message.body) {
        return nil
      }

      guard
        let data = try? JSONSerialization.data(
          withJSONObject: message.body,
          options: [.fragmentsAllowed]
        )
      else {
        return nil
      }

      return try? JSONDecoder().decode(ReadyState.self, from: data)
    }
  }
}
