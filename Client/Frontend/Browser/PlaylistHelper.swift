// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import AVKit
import Data
import BraveShared
import Shared

private let log = Logger.browserLogger

enum PlaylistItemAddedState {
    case added
    case pendingUserAction
    case existing
}

protocol PlaylistHelperDelegate: NSObject {
    func showPlaylistAlert(_ alertController: UIAlertController)
    func showPlaylistToast(info: PlaylistInfo, itemState: PlaylistItemAddedState)
    func addToPlayListActivity(info: PlaylistInfo?, itemDetected: Bool)
    func openInPlayListActivity(info: PlaylistInfo?)
    func dismissPlaylistToast(animated: Bool)
}

class PlaylistHelper: NSObject, TabContentScript {
    fileprivate weak var tab: Tab?
    public weak var delegate: PlaylistHelperDelegate?
    private var url: URL?
    private var playlistItems = Set<String>()
    private var urlObserver: NSObjectProtocol?
    private var asset: AVURLAsset?
    private static let queue = DispatchQueue(label: "com.playlisthelper.queue", qos: .userInitiated)
    
    init(tab: Tab) {
        self.tab = tab
        self.url = tab.url
        super.init()
        
        urlObserver = tab.webView?.observe(\.url, options: [.new], changeHandler: { [weak self] _, change in
            guard let self = self, let url = change.newValue else { return }
            if self.url != url {
                self.url = url
                self.playlistItems = Set<String>()
                
                self.asset?.cancelLoading()
                self.asset = nil
                
                self.delegate?.dismissPlaylistToast(animated: false)
                self.delegate?.addToPlayListActivity(info: nil, itemDetected: false)
            }
        })
        
        tab.webView?.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onLongPressedWebView(_:))).then {
            $0.delegate = self
        })
    }
    
    deinit {
        asset?.cancelLoading()
    }
    
    static func name() -> String {
        return "PlaylistHelper"
    }
    
    func scriptMessageHandlerName() -> String? {
        return "playlistHelper_\(UserScriptManager.messageHandlerTokenString)"
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard let item = PlaylistInfo.from(message: message), !item.src.isEmpty else {
            DispatchQueue.main.async {
                self.delegate?.openInPlayListActivity(info: nil)
                self.delegate?.addToPlayListActivity(info: nil, itemDetected: false)
            }
            return
        }
        
        PlaylistHelper.queue.async { [weak self] in
            guard let self = self else { return }

            if item.duration <= 0.0 && !item.detected || item.src.isEmpty || item.src.hasPrefix("data:") || item.src.hasPrefix("blob:") {
                DispatchQueue.main.async {
                    self.delegate?.openInPlayListActivity(info: nil)
                    self.delegate?.addToPlayListActivity(info: nil, itemDetected: false)
                }
                return
            }
            
            if let url = URL(string: item.src) {
                self.loadAssetPlayability(url: url) { [weak self] isPlayable in
                    guard let self = self else { return }
                    
                    if !isPlayable {
                        self.delegate?.openInPlayListActivity(info: nil)
                        self.delegate?.addToPlayListActivity(info: nil, itemDetected: false)
                        return
                    }
                    
                    if PlaylistItem.itemExists(item) {
                        self.updateItem(item)
                    } else if item.detected {
                        self.delegate?.openInPlayListActivity(info: nil)
                        self.delegate?.addToPlayListActivity(info: item, itemDetected: true)
                        self.delegate?.showPlaylistToast(info: item, itemState: .pendingUserAction)
                    } else {
                        self.promptUserForAddingItem(item)
                    }
                }
            }
        }
    }
    
    private func loadAssetPlayability(url: URL, completion: @escaping (Bool) -> Void) {
        if asset == nil {
            // We have to create an AVURLAsset here to determine if the item is playable
            // because otherwise it will add an invalid item to playlist that can't be played.
            // IE: WebM videos aren't supported so can't be played.
            // Therefore we shouldn't prompt the user to add to playlist.
            asset = AVURLAsset(url: url)
        }
        
        let isAssetPlayable = { [weak self]() -> Bool in
            guard let self = self else { return false }
            
            var error: NSError?
            let status = self.asset?.statusOfValue(forKey: "playable", error: &error)
            let isPlayable = status == .loaded
            
            if let error = error {
                log.error("Couldn't load asset's playability: \(error)")
            }
            return isPlayable
        }
        
        // Performance improvement to check the status first
        // before attempting to load the playable status
        if isAssetPlayable() {
            DispatchQueue.main.async {
                completion(true)
            }
            return
        }
        
        switch Reach().connectionStatus() {
        case .offline, .unknown:
            log.error("Couldn't load asset's playability -- Offline")
            DispatchQueue.main.async {
                // We have no other way of knowing the playable status
                // It is best to assume the item can be played
                // In the worst case, if it can't be played, it will show an error
                completion(true)
            }
        case .online:
            // Fetch the playable status asynchronously
            asset?.loadValuesAsynchronously(forKeys: ["playable"]) {
                DispatchQueue.main.async {
                    completion(isAssetPlayable())
                }
            }
        }
    }
    
    private func updateItem(_ item: PlaylistInfo) {
        self.delegate?.openInPlayListActivity(info: item)
        self.delegate?.addToPlayListActivity(info: nil, itemDetected: false)
        
        PlaylistItem.updateItem(item) {
            log.debug("Playlist Item Updated")
            
            if !self.playlistItems.contains(item.src) {
                self.playlistItems.insert(item.src)
                self.delegate?.showPlaylistToast(info: item, itemState: .existing)
            }
        }
    }
    
    private func promptUserForAddingItem(_ item: PlaylistInfo) {
        self.delegate?.openInPlayListActivity(info: nil)
        self.delegate?.addToPlayListActivity(info: item, itemDetected: true)
        
        // Has to be done otherwise it is impossible to play a video after selecting its elements
        UIMenuController.shared.hideMenu()
        
        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
        let alert = UIAlertController(
            title: Strings.PlayList.addToPlayListAlertTitle, message: Strings.PlayList.addToPlayListAlertDescription, preferredStyle: style)
        
        alert.addAction(UIAlertAction(title: Strings.PlayList.addToPlayListAlertTitle, style: .default, handler: { _ in
            // Update playlist with new items..
            PlaylistItem.addItem(item, cachedData: nil) {
                PlaylistManager.shared.autoDownload(item: item)
                
                log.debug("Playlist Item Added")
                self.delegate?.showPlaylistToast(info: item, itemState: .added)
                UIImpactFeedbackGenerator(style: .medium).bzzt()
            }
        }))
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        self.delegate?.showPlaylistAlert(alert)
    }
}

extension PlaylistHelper: UIGestureRecognizerDelegate {
    @objc
    func onLongPressedWebView(_ gestureRecognizer: UILongPressGestureRecognizer) {
        if gestureRecognizer.state == .began,
           let webView = tab?.webView,
           Preferences.Playlist.enableLongPressAddToPlaylist.value {
            let touchPoint = gestureRecognizer.location(in: webView)
            
            let token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
            let javascript = String(format: "window.onLongPressActivated_%@(%f, %f)", token, touchPoint.x, touchPoint.y)
            webView.evaluateJavaScript(javascript) // swiftlint:disable:this safe_javascript
        }
    }
    
    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
        if otherGestureRecognizer.isKind(of: UILongPressGestureRecognizer.self) {
            return true
        }
        return false
    }
    
    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldBeRequiredToFailBy otherGestureRecognizer: UIGestureRecognizer) -> Bool {
        return false
    }
}
