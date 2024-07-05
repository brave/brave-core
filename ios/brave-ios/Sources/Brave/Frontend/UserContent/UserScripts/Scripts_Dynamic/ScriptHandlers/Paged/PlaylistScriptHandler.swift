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
import WebKit
import os.log

enum PlaylistItemAddedState {
  case none
  case newItem
  case existingItem
}

protocol PlaylistScriptHandlerDelegate: NSObject {
  func updatePlaylistURLBar(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?)
  func showPlaylistPopover(tab: Tab?)
  func showPlaylistToast(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?)
  func showPlaylistAlert(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?)
  func showPlaylistOnboarding(tab: Tab?)
}

class PlaylistScriptHandler: NSObject, TabContentScript {
  fileprivate weak var tab: Tab?
  public weak var delegate: PlaylistScriptHandlerDelegate?
  private var url: URL?
  private var urlObserver: NSObjectProtocol?
  private var asset: AVURLAsset?
  private static let queue = DispatchQueue(label: "com.playlisthelper.queue", qos: .userInitiated)

  init(tab: Tab) {
    self.tab = tab
    self.url = tab.url
    super.init()

    urlObserver = tab.webView?.observe(
      \.url,
      options: [.new],
      changeHandler: { [weak self] _, change in
        guard let self = self, let url = change.newValue else { return }
        if self.url != url {
          self.url = url
          self.asset?.cancelLoading()
          self.asset = nil

          self.delegate?.updatePlaylistURLBar(tab: self.tab, state: .none, item: nil)
        }
      }
    )

    tab.webView?.addGestureRecognizer(
      UILongPressGestureRecognizer(target: self, action: #selector(onLongPressedWebView(_:))).then {
        $0.delegate = self
      }
    )
  }

  deinit {
    asset?.cancelLoading()
    delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
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

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return (nil, nil)
    }

    // If this URL is blocked from Playlist support, do nothing
    if url?.isPlaylistBlockedSiteURL == true {
      return (nil, nil)
    }

    if ReadyState.from(message: message) != nil {
      return (nil, nil)
    }

    await Self.processPlaylistInfo(
      handler: self,
      item: PlaylistInfo.from(message: message)
    )

    return (nil, nil)
  }

  @MainActor
  private class func processPlaylistInfo(handler: PlaylistScriptHandler, item: PlaylistInfo?) async
  {
    guard var item = item, !item.src.isEmpty else {
      handler.delegate?.updatePlaylistURLBar(tab: handler.tab, state: .none, item: nil)
      return
    }

    if handler.url?.baseDomain != "soundcloud.com", item.isInvisible {
      handler.delegate?.updatePlaylistURLBar(tab: handler.tab, state: .none, item: nil)
      return
    }

    // Copy the item but use the web-view's title and location instead, if available
    // This is due to a iFrames security
    let newItem = PlaylistInfo(
      name: item.name,
      src: item.src,
      pageSrc: handler.tab?.webView?.url?.absoluteString ?? item.pageSrc,
      pageTitle: handler.tab?.webView?.title ?? item.pageTitle,
      mimeType: item.mimeType,
      duration: item.duration,
      lastPlayedOffset: 0.0,
      detected: item.detected,
      dateAdded: item.dateAdded,
      tagId: item.tagId,
      order: item.order,
      isInvisible: item.isInvisible
    )

    Self.queue.async { [weak handler] in
      guard let handler = handler else { return }

      if newItem.duration <= 0.0 && !newItem.detected || newItem.src.isEmpty {
        Task { @MainActor in
          handler.delegate?.updatePlaylistURLBar(tab: handler.tab, state: .none, item: nil)
        }
        return
      }

      if URL(string: newItem.src) != nil {
        Task { @MainActor [weak handler] in
          guard let handler = handler,
            let delegate = handler.delegate
          else { return }

          if PlaylistItem.itemExists(pageSrc: newItem.pageSrc) {
            // Item already exists, so just update the database with new token or URL.
            handler.updateItem(newItem, detected: newItem.detected)
          } else if newItem.detected {
            // Automatic Detection
            delegate.updatePlaylistURLBar(tab: handler.tab, state: .newItem, item: newItem)
            delegate.showPlaylistOnboarding(tab: handler.tab)
          } else {
            // Long-Press
            delegate.showPlaylistAlert(tab: handler.tab, state: .newItem, item: newItem)
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

  private func updateItem(_ item: PlaylistInfo, detected: Bool) {
    if detected {
      self.delegate?.updatePlaylistURLBar(tab: self.tab, state: .existingItem, item: item)
    }

    PlaylistItem.updateItem(item) { [weak self] in
      guard let self = self else { return }

      if let delegate = self.delegate {
        if detected {
          delegate.updatePlaylistURLBar(tab: self.tab, state: .existingItem, item: item)
        } else {
          delegate.showPlaylistToast(tab: self.tab, state: .existingItem, item: item)
        }
      }
    }
  }
}

extension PlaylistScriptHandler: UIGestureRecognizerDelegate {
  @objc
  func onLongPressedWebView(_ gestureRecognizer: UILongPressGestureRecognizer) {
    if gestureRecognizer.state == .began,
      let webView = tab?.webView,
      Preferences.Playlist.enableLongPressAddToPlaylist.value
    {

      // If this URL is blocked from Playlist support, do nothing
      if url?.isPlaylistBlockedSiteURL == true {
        return
      }

      let touchPoint = gestureRecognizer.location(in: webView)

      webView.evaluateSafeJavaScript(
        functionName: "window.__firefox__.\(PlaylistScriptHandler.playlistLongPressed)",
        args: [touchPoint.x, touchPoint.y, Self.scriptId],
        contentWorld: Self.scriptSandbox,
        asFunction: true
      ) { _, error in

        if let error = error {
          Logger.module.error("Error executing onLongPressActivated: \(error.localizedDescription)")
        }
      }
    }
  }

  func gestureRecognizer(
    _ gestureRecognizer: UIGestureRecognizer,
    shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer
  ) -> Bool {
    if otherGestureRecognizer.isKind(of: UILongPressGestureRecognizer.self) {
      return true
    }
    return false
  }

  func gestureRecognizer(
    _ gestureRecognizer: UIGestureRecognizer,
    shouldBeRequiredToFailBy otherGestureRecognizer: UIGestureRecognizer
  ) -> Bool {
    return false
  }
}

extension PlaylistScriptHandler {
  static func getCurrentTime(
    webView: WKWebView,
    nodeTag: String,
    completion: @escaping (Double) -> Void
  ) {
    guard UUID(uuidString: nodeTag) != nil else {
      Logger.module.error("Unsanitized NodeTag.")
      return
    }

    webView.evaluateSafeJavaScript(
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

  static func stopPlayback(tab: Tab?) {
    guard let tab = tab else { return }

    tab.webView?.evaluateSafeJavaScript(
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
  static func updatePlaylistTab(tab: Tab, item: PlaylistInfo?) {
    if let handler = tab.getContentScript(name: Self.scriptName) as? PlaylistScriptHandler {
      Task { @MainActor in
        await Self.processPlaylistInfo(handler: handler, item: item)
      }
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
