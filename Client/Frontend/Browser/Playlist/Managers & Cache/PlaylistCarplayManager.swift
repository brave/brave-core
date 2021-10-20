// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import MediaPlayer
import CarPlay
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

/// Lightweight class that manages a single MediaPlayer item
/// The MediaPlayer is then passed to any controller that needs to use it.
class PlaylistCarplayManager: NSObject {
    private var carPlayStatusObservers = [Any]()
    private weak var mediaPlayer: MediaPlayer?
    private(set) var isCarPlayAvailable = false
    
    private var carPlayController: PlaylistCarplayController?
    private var carplayInterface: CPInterfaceController?
    private var carplaySessionConfiguration: CPSessionConfiguration?
    
    var browserController: BrowserViewController?
    
    var currentlyPlayingItemIndex = -1
    var currentPlaylistItem: PlaylistInfo?
    
    // When Picture-In-Picture is enabled, we need to store a reference to the controller to keep it alive, otherwise if it deallocates, the system automatically kills Picture-In-Picture.
    var playlistController: PlaylistViewController? {
        didSet {
            // TODO: REFACTOR and Decide what happens to Playlist in multiple windows in the future
            // IE: Will we show it on each window OR just the one browser controller.
            // After all, we can't play media simultaneously.
            if let selectedTab = browserController?.tabManager.selectedTab,
               let playlistItem = selectedTab.playlistItem,
               PlaylistManager.shared.index(of: playlistItem.pageSrc) == nil {
                
                browserController?.updatePlaylistURLBar(tab: selectedTab,
                                                        state: .newItem,
                                                        item: playlistItem)
            }
        }
    }
    
    // There can only ever be one instance of this class
    // Because there can only be a single AudioSession and MediaPlayer
    // in use at any given moment
    static let shared = PlaylistCarplayManager()
    
    func getCarPlayController() -> PlaylistCarplayController? {
        // On iOS 14, we use CPTemplate (Custom UI)
        // We control what gets displayed
        guard let carplayInterface = carplayInterface else {
            return nil
        }
        
        // CarPlay can be launched independently of the browser/app
        // CarPlay utilizes the database, and since `SceneDelegate` is never invoked
        // We must manually invoke the database initialization here.
        DataController.shared.initializeOnce()
        
        // REFACTOR to support multiple windows.
        // OR find a way to get WebKit to load `Youtube` and other sites WITHOUT having to be in the view hierarchy..
        var currentWindow = browserController?.view.window
        
        // BrowserController can actually be null when CarPlay is launched by the system
        // The only such window that will be available is the actual CarPlay window
        // So that's the window we need to use
        // This does NOT break Multi-Window because we don't actually have multiple windows running
        // when there is no browser controller. There is only a single CarPlay window.
        if currentWindow == nil {
            currentWindow = UIApplication.shared.connectedScenes
                .filter({$0.activationState == .foregroundActive})
                .compactMap({$0 as? UIWindowScene})
                .first?.windows
                .filter({$0.isKeyWindow}).first
        }
        
        // If there is no media player, create one,
        // pass it to the car-play controller
        let mediaPlayer = self.mediaPlayer ?? MediaPlayer()
        let mediaStreamer = PlaylistMediaStreamer(playerView: currentWindow ?? UIView(),
                                                  certStore: browserController?.profile.certStore)
        
        // Construct the CarPlay UI
        let carPlayController = PlaylistCarplayController(mediaStreamer: mediaStreamer,
                                                          player: mediaPlayer,
                                                          interfaceController: carplayInterface)
        self.mediaPlayer = mediaPlayer
        return carPlayController
    }
    
    func getPlaylistController(tab: Tab?, initialItem: PlaylistInfo?, initialItemPlaybackOffset: Double) -> PlaylistViewController {
        
        // If background playback is enabled, tabs will continue to play media
        // Even if another controller is presented and even when PIP is enabled in playlist.
        // Therefore we need to stop the page/tab from playing when using playlist.
        if Preferences.General.mediaAutoBackgrounding.value {
            tab?.stopMediaPlayback()
        }
        
        // If there is no media player, create one,
        // pass it to the play-list controller
        let mediaPlayer = self.mediaPlayer ?? MediaPlayer()
        
        let playlistController = self.playlistController ??
            PlaylistViewController(openInNewTab: browserController?.openURLInNewTab,
                                   profile: browserController?.profile,
                                   mediaPlayer: mediaPlayer,
                                   initialItem: initialItem,
                                   initialItemPlaybackOffset: initialItemPlaybackOffset)
        self.mediaPlayer = mediaPlayer
        return playlistController
    }
    
    func getPlaylistController(tab: Tab?, completion: @escaping (PlaylistViewController) -> Void) {
        if let playlistController = self.playlistController {
            return completion(playlistController)
        }
        
        if let tab = tab,
           let item = tab.playlistItem,
           let webView = tab.webView,
           let tag = tab.playlistItem?.tagId {
            PlaylistHelper.getCurrentTime(webView: webView, nodeTag: tag) { [unowned self] currentTime in
                completion(self.getPlaylistController(tab: tab,
                                                      initialItem: item,
                                                      initialItemPlaybackOffset: currentTime))
            }
        } else {
            return completion(getPlaylistController(tab: tab,
                                                    initialItem: nil,
                                                    initialItemPlaybackOffset: 0.0))
        }
    }
    
    private func attemptInterfaceConnection(isCarPlayAvailable: Bool) {
        self.isCarPlayAvailable = isCarPlayAvailable
        
        // If there is no media player, create one,
        // pass it to the carplay controller
        if isCarPlayAvailable {
            // Protect against reentrancy.
            if carPlayController == nil {
                carPlayController = getCarPlayController()
            }
        } else {
            carPlayController = nil
            mediaPlayer = nil
        }

        // Sometimes the `endpointAvailable` WILL RETURN TRUE!
        // Even when the car is NOT connected.
        log.debug("CARPLAY CONNECTED: \(isCarPlayAvailable)")
    }
}

extension PlaylistCarplayManager: CPSessionConfigurationDelegate {
    func connect(interfaceController: CPInterfaceController) {
        carplayInterface = interfaceController
        carplaySessionConfiguration = CPSessionConfiguration(delegate: self)
        
        isCarPlayAvailable = true
        attemptInterfaceConnection(isCarPlayAvailable: true)
    }
    
    func disconnect(interfaceController: CPInterfaceController) {
        isCarPlayAvailable = false
        carplayInterface = nil
        carplayInterface?.delegate = nil
        attemptInterfaceConnection(isCarPlayAvailable: false)
    }
    
    func sessionConfiguration(_ sessionConfiguration: CPSessionConfiguration,
                              limitedUserInterfacesChanged limitedUserInterfaces: CPLimitableUserInterface) {
        log.debug("Limited UI changed: \(limitedUserInterfaces)")
    }
}
