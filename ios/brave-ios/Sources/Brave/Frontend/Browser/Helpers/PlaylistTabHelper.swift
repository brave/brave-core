// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Data
import OSLog
import Preferences
import UIKit
@_spi(ChromiumWebViewAccess) import Web

enum PlaylistItemAddedState {
  case none
  case newItem
  case existingItem
}

protocol PlaylistTabHelperDelegate: AnyObject {
  func updatePlaylistURLBar(
    tab: (any TabState)?,
    state: PlaylistItemAddedState,
    item: PlaylistInfo?
  )
  func showPlaylistAlert(tab: (any TabState)?, state: PlaylistItemAddedState, item: PlaylistInfo?)
  func showPlaylistOnboarding(tab: (any TabState)?)
}

extension TabDataValues {
  private struct PlaylistTabHelperKey: TabDataKey {
    static var defaultValue: PlaylistTabHelper?
  }
  var playlist: PlaylistTabHelper? {
    get { self[PlaylistTabHelperKey.self] }
    set { self[PlaylistTabHelperKey.self] = newValue }
  }
}

class PlaylistTabHelper: NSObject, TabObserver, PlaylistTabHelperBridge {
  weak var delegate: PlaylistTabHelperDelegate?
  private weak var tab: (any TabState)?
  private var url: URL?
  private static let queue = DispatchQueue(label: "com.playlisthelper.queue", qos: .userInitiated)

  init(tab: some TabState, delegate: PlaylistTabHelperDelegate?) {
    self.tab = tab
    self.url = tab.visibleURL
    self.delegate = delegate
    super.init()
    tab.addObserver(self)

    let longPress = UILongPressGestureRecognizer(target: self, action: #selector(longPressed(_:)))
    longPress.delegate = self
    tab.view.addGestureRecognizer(longPress)
  }

  /// Processes a detected media item, updating the URL bar and prompting the
  /// user as needed. Shared by the legacy `PlaylistScript` and the
  /// `PlaylistJavaScriptFeature`, both of which deliver the same payload.
  func processPlaylistInfo(item: PlaylistInfo?) {
    guard let tab = self.tab else { return }

    // If this URL is blocked from Playlist support, do nothing
    if url?.isPlaylistBlockedSiteURL == true {
      return
    }

    guard var item = item, !item.src.isEmpty else {
      DispatchQueue.main.async { [weak self] in
        self?.delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
      }
      return
    }

    if url?.baseDomain != "soundcloud.com", item.isInvisible {
      DispatchQueue.main.async { [weak self] in
        self?.delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
      }
      return
    }

    // Copy the item but use the web-view's title and location instead, if available
    // This is due to a iFrames security
    item.pageSrc = tab.visibleURL?.absoluteString ?? item.pageSrc
    item.pageTitle = tab.title ?? item.pageTitle
    item.lastPlayedOffset = 0.0

    Self.queue.async { [weak self] in
      guard let self = self else { return }

      if item.duration <= 0.0 && !item.detected || item.src.isEmpty {
        DispatchQueue.main.async { [weak self] in
          self?.delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
        }
        return
      }

      if URL(string: item.src) != nil {
        DispatchQueue.main.async { [weak self] in
          guard let self = self,
            let delegate = self.delegate
          else { return }

          if PlaylistItem.itemExists(pageSrc: item.pageSrc) {
            // Item already exists, so just update the database with new token or URL.
            self.updateItem(item, detected: item.detected)
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

  private func updateItem(_ item: PlaylistInfo, detected: Bool) {
    guard let tab = self.tab else { return }
    if detected {
      delegate?.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
    }

    PlaylistItem.updateItem(item) { [weak self] in
      guard let self = self, let tab = self.tab else { return }

      // We need to use the Database version of this object
      // because when the fallback streamer updates the object, it uses the database ID.
      // When the download starts, it uses the database ID.
      // If we suddenly change the ID, downloads and updates get out of wack
      var item = item

      // Use the ID that it was saved as in the database, rather than the Javascript ID
      item.tagId = $0

      if detected, let delegate = self.delegate {
        delegate.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
      }
    }
  }

  /// Queries the current playback time of the media element tagged with
  /// `nodeTag`. Uses `PlaylistJavaScriptFeature` when the new web view
  /// configuration is enabled, otherwise the legacy `PlaylistScript`.
  func getCurrentTime(nodeTag: String, completion: @escaping (Double) -> Void) {
    guard let tab = self.tab else {
      completion(0.0)
      return
    }

    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      guard let webView = BraveWebView.from(tab: tab) else {
        completion(0.0)
        return
      }
      webView.playlistCurrentTime(forTag: nodeTag) { currentTime in
        DispatchQueue.main.async {
          completion(currentTime)
        }
      }
      return
    }

    guard UUID(uuidString: nodeTag) != nil else {
      Logger.module.error("Unsanitized NodeTag.")
      return
    }

    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(PlaylistScriptHandler.mediaCurrentTimeFromTag)",
      args: [nodeTag, PlaylistScriptHandler.scriptId],
      contentWorld: PlaylistScriptHandler.scriptSandbox,
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

  /// Receives a media item detected by `PlaylistJavaScriptFeature`. The payload
  /// mirrors `PlaylistInfo` so it can be processed identically to the legacy
  /// `PlaylistScript` messages.
  func onMediaDetected(_ info: [String: Any]) {
    processPlaylistInfo(item: PlaylistInfo.from(dictionary: info))
  }

  // MARK: - TabObserver

  func tabDidCreateWebView(_ tab: some TabState) {
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      BraveWebView.from(tab: tab)?.setPlaylistHelper(self)
    }
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    url = tab.visibleURL
    delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  @objc private func longPressed(_ gestureRecognizer: UILongPressGestureRecognizer) {
    if gestureRecognizer.state == .began,
      let tab,
      Preferences.Playlist.enableLongPressAddToPlaylist.value
    {

      // If this URL is blocked from Playlist support, do nothing
      if tab.visibleURL?.isPlaylistBlockedSiteURL == true {
        return
      }

      let touchPoint = gestureRecognizer.location(in: tab.view)

      if FeatureList.kUseProfileWebViewConfiguration.enabled {
        BraveWebView.from(tab: tab)?.playlistLongPressed(at: touchPoint)
        return
      }

      tab.evaluateJavaScript(
        functionName: "window.__firefox__.\(PlaylistScriptHandler.playlistLongPressed)",
        args: [touchPoint.x, touchPoint.y, PlaylistScriptHandler.scriptId],
        contentWorld: PlaylistScriptHandler.scriptSandbox,
        asFunction: true
      ) { _, error in
        if let error = error {
          Logger.module.error("Error executing onLongPressActivated: \(error.localizedDescription)")
        }
      }
    }
  }
}

extension PlaylistTabHelper: UIGestureRecognizerDelegate {
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
