// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import MediaPlayer
import Shared
import Data
import BraveShared

private let log = Logger.browserLogger

/// Lightweight class that manages a single MediaPlayer item
/// The MediaPlayer is then passed to any controller that needs to use it.
class PlaylistCarplayManager: NSObject {
    private var carPlayStatusObservers = [Any]()
    private let contentManager = MPPlayableContentManager.shared()
    private var carPlayController: PlaylistCarplayController?
    private weak var mediaPlayer: MediaPlayer?
    private(set) var isCarPlayAvailable = false
    
    var currentlyPlayingItemIndex = -1
    var currentPlaylistItem: PlaylistInfo?
    var browserController: BrowserViewController?
    
    // When Picture-In-Picture is enabled, we need to store a reference to the controller to keep it alive, otherwise if it deallocates, the system automatically kills Picture-In-Picture.
    var playlistController: PlaylistViewController?
    
    // There can only ever be one instance of this class
    // Because there can only be a single AudioSession and MediaPlayer
    // in use at any given moment
    static let shared = PlaylistCarplayManager()
    
    private override init() {
        super.init()
        
        #if targetEnvironment(simulator)
        // Force CarPlay on the simulator
        // This is because we don't actually know if the CarPlay simulator is showing or not
        // and we don't get notified when it disconnects :(
        attemptInterfaceConnection(isCarPlayAvailable: true)
        #else
        // We need to observe when CarPlay is connected
        // That way, we can  determine where the controls are coming from for Playlist
        // OR determine where the AudioSession is outputting
        
        // We need to observe the audio route because sometimes the car will be disconnected
        // and contentManager.context.endpointAvailable will still return true!
        carPlayStatusObservers.append(NotificationCenter.default.addObserver(forName: AVAudioSession.routeChangeNotification, object: nil, queue: .main) { [weak self] _ in
            
            let hasCarPlay = AVAudioSession.sharedInstance().currentRoute.outputs.contains(where: { $0.portType == .carAudio })
            self?.attemptInterfaceConnection(isCarPlayAvailable: hasCarPlay)
        })
        
        // Using publisher for this crashes no matter what!
        // The moment you call `sink` on the publisher, it will crash.
        // Seems like a bug in iOS itself.
        // We observe the contentManager.context.endpointAvailable to determine when to create
        // a carplay handler
        carPlayStatusObservers.append(contentManager.observe(\.context) { [weak self] contentManager, _ in
            self?.carPlayStatusObservers.append(contentManager.context.observe(\.endpointAvailable) { [weak self] context, change in
                self?.attemptInterfaceConnection(isCarPlayAvailable: context.endpointAvailable)
            })
        })
        
        // This is needed because the notifications for carplay doesn't get posted initial
        // until you actually attempt to use the AudioSession or Context
        let hasCarPlay = AVAudioSession.sharedInstance().currentRoute.outputs.contains(where: { $0.portType == .carAudio })
        let hasCarPlayEndpoint = contentManager.context.endpointAvailable
        attemptInterfaceConnection(isCarPlayAvailable: hasCarPlay || hasCarPlayEndpoint)
        #endif
    }
    
    func getCarPlayController() -> PlaylistCarplayController {
        // If there is no media player, create one,
        // pass it to the car-play controller
        let mediaPlayer = self.mediaPlayer ?? MediaPlayer()
        let carPlayController = PlaylistCarplayController(browser: browserController, player: mediaPlayer, contentManager: contentManager)
        self.mediaPlayer = mediaPlayer
        return carPlayController
    }
    
    func getPlaylistController(initialItem: PlaylistInfo?, initialItemPlaybackOffset: Double) -> PlaylistViewController {
        
        // If background playback is enabled, tabs will continue to play media
        // Even if another controller is presented and even when PIP is enabled in playlist.
        // Therefore we need to stop the page/tab from playing when using playlist.
        if Preferences.General.mediaAutoBackgrounding.value {
            browserController?.tabManager.allTabs.forEach({
                PlaylistHelper.stopPlayback(tab: $0)
            })
        }
        
        // If there is no media player, create one,
        // pass it to the play-list controller
        let mediaPlayer = self.mediaPlayer ?? MediaPlayer()
        
        let playlistController = self.playlistController ??
            PlaylistViewController(mediaPlayer: mediaPlayer,
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
                DispatchQueue.main.async {
                    completion(self.getPlaylistController(initialItem: item,
                                                     initialItemPlaybackOffset: currentTime))
                }
            }
        } else {
            return completion(getPlaylistController(initialItem: nil,
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
                carPlayController = self.getCarPlayController()
            }
        } else {
            carPlayController = nil
            mediaPlayer = nil
        }

        // Sometimes the `endpointAvailable` WILL RETURN TRUE!
        // Even when the car is NOT connected.
        log.debug("CARPLAY CONNECTED: \(isCarPlayAvailable) -- \(contentManager.context.endpointAvailable)")
    }
}
