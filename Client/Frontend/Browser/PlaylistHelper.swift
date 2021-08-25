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
    case none
    case newItem
    case existingItem
}

protocol PlaylistHelperDelegate: NSObject {
    func updatePlaylistURLBar(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?)
    func showPlaylistPopover(tab: Tab?, state: PlaylistPopoverState)
    func showPlaylistToast(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?)
    func showPlaylistAlert(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?)
    func showPlaylistOnboarding(tab: Tab?)
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
                
                self.delegate?.updatePlaylistURLBar(tab: self.tab, state: .none, item: nil)
            }
        })
        
        tab.webView?.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onLongPressedWebView(_:))).then {
            $0.delegate = self
        })
    }
    
    deinit {
        asset?.cancelLoading()
        delegate?.updatePlaylistURLBar(tab: tab, state: .none, item: nil)
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
                self.delegate?.updatePlaylistURLBar(tab: self.tab, state: .none, item: nil)
            }
            return
        }
        
        PlaylistHelper.queue.async { [weak self] in
            guard let self = self else { return }

            if item.duration <= 0.0 && !item.detected || item.src.isEmpty || item.src.hasPrefix("data:") || item.src.hasPrefix("blob:") {
                DispatchQueue.main.async {
                    self.delegate?.updatePlaylistURLBar(tab: self.tab, state: .none, item: nil)
                }
                return
            }
            
            if let url = URL(string: item.src) {
                self.loadAssetPlayability(url: url) { [weak self] isPlayable in
                    guard let self = self,
                          let delegate = self.delegate else { return }
                    
                    if !isPlayable {
                        delegate.updatePlaylistURLBar(tab: self.tab, state: .none, item: nil)
                        return
                    }
                    
                    if PlaylistItem.itemExists(item) {
                        // Item already exists, so just update the database with new token or URL.
                        self.updateItem(item, detected: item.detected)
                    } else if item.detected {
                        // Automatic Detection
                        delegate.updatePlaylistURLBar(tab: self.tab, state: .newItem, item: item)
                        delegate.showPlaylistOnboarding(tab: self.tab)
                    } else {
                        // Long-Press
                        delegate.showPlaylistAlert(tab: self.tab, state: .newItem, item: item)
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
    
    private func updateItem(_ item: PlaylistInfo, detected: Bool) {
        if detected {
            self.delegate?.updatePlaylistURLBar(tab: self.tab, state: .existingItem, item: item)
        }
        
        PlaylistItem.updateItem(item) { [weak self] in
            guard let self = self else { return }
            
            log.debug("Playlist Item Updated")
            
            if !self.playlistItems.contains(item.src) {
                self.playlistItems.insert(item.src)
                
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

extension PlaylistHelper {
    static func getCurrentTime(webView: WKWebView, nodeTag: String, completion: @escaping (Double) -> Void) {
        let token = UserScriptManager.securityTokenString
        let javascript = String(format: "window.mediaCurrentTimeFromTag_%@('%@')", token, nodeTag)

        // swiftlint:disable:next safe_javascript
        webView.evaluateJavaScript(javascript, completionHandler: { value, error in
            if let error = error {
                log.error("Error Retrieving Playlist Page Media Current Time: \(error)")
            }
            
            DispatchQueue.main.async {
                if let value = value as? Double {
                    completion(value)
                } else {
                    completion(0.0)
                }
            }
        })
    }
}
