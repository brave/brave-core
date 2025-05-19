// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import OSLog
import Preferences
import UIKit
import Web

extension TabDataValues {
  private struct PlaylistTabHelperKey: TabDataKey {
    static var defaultValue: PlaylistTabHelper?
  }
  var playlist: PlaylistTabHelper? {
    get { self[PlaylistTabHelperKey.self] }
    set { self[PlaylistTabHelperKey.self] = newValue }
  }
}

class PlaylistTabHelper: NSObject, TabObserver {
  private weak var tab: (any TabState)?

  init(tab: some TabState) {
    self.tab = tab
    super.init()
    tab.addObserver(self)
  }

  func tabDidCreateWebView(_ tab: some TabState) {
    let longPress = UILongPressGestureRecognizer(target: self, action: #selector(longPressed(_:)))
    longPress.delegate = self
    tab.view.addGestureRecognizer(longPress)
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
      if tab.url?.isPlaylistBlockedSiteURL == true {
        return
      }

      let touchPoint = gestureRecognizer.location(in: tab.view)

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
