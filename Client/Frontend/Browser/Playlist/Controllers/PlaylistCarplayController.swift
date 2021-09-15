// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import CarPlay
import MediaPlayer
import Data
import BraveShared
import Shared

private let log = Logger.browserLogger

class PlaylistCarplayController: NSObject {
    private let player: MediaPlayer
    private let mediaStreamer: PlaylistMediaStreamer
    private let contentManager: MPPlayableContentManager
    private var playerStateObservers = Set<AnyCancellable>()
    private var assetStateObservers = Set<AnyCancellable>()
    private var assetLoadingStateObservers = Set<AnyCancellable>()
    private var playlistObservers = Set<AnyCancellable>()
    private var playlistItemIds = [String]()
    private weak var browser: BrowserViewController?
    
    init(browser: BrowserViewController?, player: MediaPlayer, contentManager: MPPlayableContentManager) {
        self.browser = browser
        self.player = player
        self.contentManager = contentManager
        self.mediaStreamer = PlaylistMediaStreamer(playerView: browser?.view ?? UIView())
        super.init()
        
        observePlayerStates()
        observePlaylistStates()
        PlaylistManager.shared.reloadData()
        
        playlistItemIds = PlaylistManager.shared.allItems.map { $0.pageSrc }
        
        contentManager.dataSource = self
        contentManager.delegate = self
        
        DispatchQueue.main.async {
            contentManager.beginUpdates()
            contentManager.endUpdates()
            contentManager.reloadData()
            
            // Workaround to see carplay NowPlaying on the simulator
            #if targetEnvironment(simulator)
            UIApplication.shared.endReceivingRemoteControlEvents()
            UIApplication.shared.beginReceivingRemoteControlEvents()
            #endif
        }
    }
    
    func observePlayerStates() {
        player.publisher(for: .play).sink { _ in
            MPNowPlayingInfoCenter.default().playbackState = .playing
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .pause).sink { _ in
            MPNowPlayingInfoCenter.default().playbackState = .paused
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .stop).sink { _ in
            MPNowPlayingInfoCenter.default().playbackState = .stopped
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .changePlaybackRate).sink { [weak self] _ in
            MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = self?.player.rate
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .previousTrack).sink { [weak self] _ in
            self?.onPreviousTrack(isUserInitiated: true)
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .nextTrack).sink { [weak self] _ in
            self?.onNextTrack(isUserInitiated: true)
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .finishedPlaying).sink { [weak self] event in
            event.mediaPlayer.pause()
            event.mediaPlayer.seek(to: .zero)
            self?.onNextTrack(isUserInitiated: false)
        }.store(in: &playerStateObservers)
    }
    
    func observePlaylistStates() {
        let reloadData = { [weak self] in
            guard let self = self else { return }
            
            self.playlistItemIds = PlaylistManager.shared.allItems.map { $0.pageSrc }
            self.contentManager.beginUpdates()
            self.contentManager.endUpdates()
            
            // FOR SOME REASON ON SIMULATOR,
            // CALLING THIS DOES NOT UPDATE THE UI!!
            // WE WILL GET A CRASH ON SIMULATOR HERE!
            // SEE: https://developer.apple.com/forums/thread/112101
            self.contentManager.reloadData()
        }
        
        PlaylistManager.shared.contentWillChange
        .sink { _ in
            reloadData()
        }.store(in: &playlistObservers)
        
        PlaylistManager.shared.contentDidChange
        .sink { _ in
            reloadData()
        }.store(in: &playlistObservers)
        
        PlaylistManager.shared.objectDidChange
        .sink { _ in
            reloadData()
        }.store(in: &playlistObservers)
        
        PlaylistManager.shared.downloadStateChanged
        .sink { _ in
            reloadData()
        }.store(in: &playlistObservers)
    }
}

extension PlaylistCarplayController: MPPlayableContentDelegate {
    func playableContentManager(_ contentManager: MPPlayableContentManager, didUpdate context: MPPlayableContentManagerContext) {
        
        log.debug("CAR PLAY CONNECTED: \(context.endpointAvailable)")
    }
    
    func playableContentManager(_ contentManager: MPPlayableContentManager, initiatePlaybackOfContentItemAt indexPath: IndexPath, completionHandler: @escaping (Error?) -> Void) {
        
        if indexPath.count == 2 {
            // Item Section
            DispatchQueue.main.async {
                guard let mediaItem = PlaylistManager.shared.itemAtIndex(indexPath.item) else {
                    completionHandler(nil)
                    return
                }
                
                self.contentManager.nowPlayingIdentifiers = [mediaItem.pageSrc]
                self.playItem(item: mediaItem) { [weak self] error in
                    PlaylistCarplayManager.shared.currentPlaylistItem = nil
                    
                    switch error {
                    case .other(let error):
                        log.error(error)
                        completionHandler("Unknown Error")
                    case .expired:
                        completionHandler(Strings.PlayList.expiredAlertDescription)
                    case .none:
                        guard let self = self else {
                            completionHandler("Unknown Error")
                            return
                        }
                        
                        PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.item
                        PlaylistCarplayManager.shared.currentPlaylistItem = mediaItem
                        PlaylistMediaStreamer.setNowPlayingMediaArtwork(artwork: self.contentItem(at: indexPath)?.artwork)
                        
                        completionHandler(nil)
                    case .cancelled:
                        log.debug("User Cancelled Playlist playback")
                        completionHandler(nil)
                    }
                    
                    // Workaround to see carplay NowPlaying on the simulator
                    #if targetEnvironment(simulator)
                    DispatchQueue.main.async {
                        UIApplication.shared.endReceivingRemoteControlEvents()
                        UIApplication.shared.beginReceivingRemoteControlEvents()
                    }
                    #endif
                }
            }
        } else {
            // Tab Section
            completionHandler(nil)
        }
    }
    
    func beginLoadingChildItems(at indexPath: IndexPath, completionHandler: @escaping (Error?) -> Void) {
        // For some odd reason, this is never called in the simulator.
        // It is only called in the car and that's fine.
        completionHandler(nil)
    }
}

extension PlaylistCarplayController: MPPlayableContentDataSource {
    func numberOfChildItems(at indexPath: IndexPath) -> Int {
        if indexPath.indices.count == 0 {
            return 1 // 1 Tab.
        }
        
        return playlistItemIds.count
    }
    
    func childItemsDisplayPlaybackProgress(at indexPath: IndexPath) -> Bool {
        true
    }
    
    func contentItem(at indexPath: IndexPath) -> MPContentItem? {
        // Tab Section
        if indexPath.count == 1 {
            let item = MPContentItem(identifier: "BravePlaylist")
            item.title = Strings.PlayList.playlistCarplayTitle
            item.isContainer = true
            item.isPlayable = false
            let imageIcon = #imageLiteral(resourceName: "settings-shields")
            item.artwork = MPMediaItemArtwork(boundsSize: imageIcon.size, requestHandler: { _ -> UIImage in
                return imageIcon
            })
            return item
        }

        if indexPath.count == 2 {
            // Items section
            guard let itemId = playlistItemIds[safe: indexPath.row] else {
                return nil
            }
            
            let item = MPContentItem(identifier: itemId)
            
            DispatchQueue.main.async {
                guard let mediaItem = PlaylistManager.shared.itemAtIndex(indexPath.item) else {
                    return
                }

                let cacheState = PlaylistManager.shared.state(for: mediaItem.pageSrc)
                item.title = mediaItem.name
                item.subtitle = mediaItem.pageSrc
                item.isPlayable = true
                item.isStreamingContent = cacheState != .downloaded
                item.loadThumbnail(for: mediaItem)
            }
            return item
        }

        return nil
    }
}

extension PlaylistCarplayController {
    func onPreviousTrack(isUserInitiated: Bool) {
        if PlaylistCarplayManager.shared.currentlyPlayingItemIndex <= 0 {
            return
        }
        
        let index = PlaylistCarplayManager.shared.currentlyPlayingItemIndex - 1
        if index < PlaylistManager.shared.numberOfAssets,
           let item = PlaylistManager.shared.itemAtIndex(index) {
            PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
            playItem(item: item) { [weak self] error in
                guard let self = self else { return }
                
                switch error {
                case .other(let err):
                    log.error(err)
                    self.displayLoadingResourceError()
                case .expired:
                    self.displayExpiredResourceError(item: item)
                case .none:
                    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
                    self.updateLastPlayedItem(item: item)
                case .cancelled:
                    log.debug("User Cancelled Playlist Playback")
                }
            }
        }
    }
    
    func onNextTrack(isUserInitiated: Bool) {
        let assetCount = PlaylistManager.shared.numberOfAssets
        let isAtEnd = PlaylistCarplayManager.shared.currentlyPlayingItemIndex >= assetCount - 1
        var index = PlaylistCarplayManager.shared.currentlyPlayingItemIndex

        switch player.repeatState {
        case .none:
            if isAtEnd {
                player.pictureInPictureController?.delegate = nil
                player.pictureInPictureController?.stopPictureInPicture()
                player.stop()

                PlaylistCarplayManager.shared.playlistController = nil
                return
            }
            index += 1
        case .repeatOne:
            player.seek(to: 0.0)
            player.play()
            return
        case .repeatAll:
            index = isAtEnd ? 0 : index + 1
        }

        if index >= 0,
           let item = PlaylistManager.shared.itemAtIndex(index) {
            self.playItem(item: item) { [weak self] error in
                guard let self = self else { return }

                switch error {
                case .other(let err):
                    log.error(err)
                    self.displayLoadingResourceError()
                case .expired:
                    if isUserInitiated || self.player.repeatState == .repeatOne || assetCount <= 1 {
                        self.displayExpiredResourceError(item: item)
                    } else {
                        DispatchQueue.main.async {
                            PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
                            self.onNextTrack(isUserInitiated: isUserInitiated)
                        }
                    }
                case .none:
                    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
                    self.updateLastPlayedItem(item: item)
                case .cancelled:
                    log.debug("User Cancelled Playlist Playback")
                }
            }
        }
    }
    
    private func play() {
        player.play()
    }
    
    private func pause() {
        player.pause()
    }
    
    private func stop() {
        player.stop()
    }
    
    private func seekBackwards() {
        player.seekBackwards()
    }
    
    private func seekForwards() {
        player.seekForwards()
    }
    
    private func seek(to time: TimeInterval) {
        player.seek(to: time)
    }
    
    func seek(relativeOffset: Float) {
        if let currentItem = player.currentItem {
            let seekTime = CMTimeMakeWithSeconds(Float64(CGFloat(relativeOffset) * CGFloat(currentItem.asset.duration.value) / CGFloat(currentItem.asset.duration.timescale)), preferredTimescale: currentItem.currentTime().timescale)
            seek(to: seekTime.seconds)
        }
    }
    
    func load(url: URL, autoPlayEnabled: Bool) -> AnyPublisher<Void, Error> {
        load(asset: AVURLAsset(url: url), autoPlayEnabled: autoPlayEnabled)
    }
    
    func load(asset: AVURLAsset, autoPlayEnabled: Bool) -> AnyPublisher<Void, Error> {
        assetLoadingStateObservers.removeAll()
        player.stop()
        
        return Future { [weak self] resolver in
            guard let self = self else {
                resolver(.failure("User Cancelled Playback"))
                return
            }
            
            self.player.load(asset: asset)
            .receive(on: RunLoop.main)
            .sink(receiveCompletion: { status in
                switch status {
                case .failure(let error):
                    resolver(.failure(error))
                case .finished:
                    break
                }
            }, receiveValue: { [weak self] isNewItem in
                guard let self = self else {
                    resolver(.failure("User Cancelled Playback"))
                    return
                }
                
                guard self.player.currentItem != nil else {
                    resolver(.failure("Couldn't load playlist item"))
                    return
                }
                
                // We are playing the same item again..
                if !isNewItem {
                    self.pause()
                    self.seek(relativeOffset: 0.0) // Restart playback
                    self.play()
                    resolver(.success(Void()))
                    return
                }
                
                // Track-bar
                if autoPlayEnabled {
                    self.play() // Play the new item
                    resolver(.success(Void()))
                }
            }).store(in: &self.assetLoadingStateObservers)
        }.eraseToAnyPublisher()
    }
    
    func playItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?) {
        assetLoadingStateObservers.removeAll()
        assetStateObservers.removeAll()

        // If the item is cached, load it from the cache and play it.
        let cacheState = PlaylistManager.shared.state(for: item.pageSrc)
        if cacheState != .invalid {
            if let index = PlaylistManager.shared.index(of: item.pageSrc),
               let asset = PlaylistManager.shared.assetAtIndex(index) {
                load(asset: asset, autoPlayEnabled: true)
                .handleEvents(receiveCancel: {
                    PlaylistMediaStreamer.clearNowPlayingInfo()
                    completion?(.cancelled)
                })
                .sink(receiveCompletion: { status in
                    switch status {
                    case .failure(let error):
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.other(error))
                    case .finished:
                        break
                    }
                }, receiveValue: { [weak self] _ in
                    guard let self = self else {
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.cancelled)
                        return
                    }
                    
                    PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
                    completion?(.none)
                }).store(in: &assetLoadingStateObservers)
            } else {
                completion?(.expired)
            }
            return
        }
        
        // The item is not cached so we should attempt to stream it
        streamItem(item: item, completion: completion)
    }
    
    func streamItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?) {
        assetStateObservers.removeAll()
        
        mediaStreamer.loadMediaStreamingAsset(item)
        .handleEvents(receiveCancel: {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(.cancelled)
        })
        .sink(receiveCompletion: { status in
            switch status {
            case .failure(let error):
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(error)
            case .finished:
                break
            }
        }, receiveValue: { [weak self] _ in
            guard let self = self else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(.cancelled)
                return
            }
            
            // Item can be streamed, so let's retrieve its URL from our DB
            guard let index = PlaylistManager.shared.index(of: item.pageSrc),
                  let item = PlaylistManager.shared.itemAtIndex(index) else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(.expired)
                return
            }
            
            // Attempt to play the stream
            if let url = URL(string: item.src) {
                self.load(url: url, autoPlayEnabled: true)
                .handleEvents(receiveCancel: {
                    PlaylistMediaStreamer.clearNowPlayingInfo()
                    completion?(.cancelled)
                })
                .sink(receiveCompletion: { status in
                    switch status {
                    case .failure(let error):
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.other(error))
                    case .finished:
                        break
                    }
                }, receiveValue: { [weak self] _ in
                    guard let self = self else {
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.cancelled)
                        return
                    }
                    
                    PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
                    completion?(.none)
                }).store(in: &self.assetLoadingStateObservers)
                log.debug("Playing Live Video: \(self.player.isLiveMedia)")
            } else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(.expired)
            }
        }).store(in: &assetStateObservers)
    }
    
    func updateLastPlayedItem(item: PlaylistInfo) {
        Preferences.Playlist.lastPlayedItemUrl.value = item.pageSrc
        
        if let playTime = player.currentItem?.currentTime(),
           Preferences.Playlist.playbackLeftOff.value {
            Preferences.Playlist.lastPlayedItemTime.value = playTime.seconds
        } else {
            Preferences.Playlist.lastPlayedItemTime.value = 0.0
        }
    }
    
    func displayExpiredResourceError(item: PlaylistInfo?) {
        if let item = item {
            let alert = UIAlertController(title: Strings.PlayList.expiredAlertTitle,
                                          message: Strings.PlayList.expiredAlertDescription, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.PlayList.reopenButtonTitle, style: .default, handler: { [weak self] _ in
                guard let browser = self?.browser else { return }
                
                if let url = URL(string: item.pageSrc) {
                    browser.dismiss(animated: true, completion: nil)
                    browser.openURLInNewTab(url, isPrivileged: false)
                }
            }))
            alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
            browser?.present(alert, animated: true, completion: nil)
        } else {
            let alert = UIAlertController(title: Strings.PlayList.expiredAlertTitle,
                                          message: Strings.PlayList.expiredAlertDescription, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
            browser?.present(alert, animated: true, completion: nil)
        }
    }
    
    func displayLoadingResourceError() {
        let alert = UIAlertController(
            title: Strings.PlayList.sorryAlertTitle, message: Strings.PlayList.loadResourcesErrorAlertDescription, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
        browser?.present(alert, animated: true, completion: nil)
    }
}

// MPContentItem has no way of storing a thumbnail with it.
// So to do that, we need associated values where we store its renderer.
// The next time it attempts to retrieve its thumbnail, we return it from the renderer.
// Otherwise it will constantly make requests for thumbnail :(
extension MPContentItem {
    
    func loadThumbnail(for mediaItem: PlaylistInfo) {
        if thumbnailRenderer != nil {
            return
        }
        
        guard let assetUrl = URL(string: mediaItem.src),
              let favIconUrl = URL(string: mediaItem.pageSrc) else {
            return
        }
        
        thumbnailRenderer = PlaylistThumbnailRenderer()
        thumbnailRenderer?.loadThumbnail(assetUrl: assetUrl,
                                         favIconUrl: favIconUrl,
                                         completion: { [weak self] image in
                                            guard let self = self else { return }
                                            
                                            let image = image ?? #imageLiteral(resourceName: "settings-shields")
                                            self.artwork = MPMediaItemArtwork(boundsSize: image.size, requestHandler: { _ -> UIImage in
                                                return image
                                            })
                                         })
    }
    
    private struct AssociatedKeys {
        static var thumbnailRenderer: Int = 0
    }
    
    private var thumbnailRenderer: PlaylistThumbnailRenderer? {
        get { objc_getAssociatedObject(self, &AssociatedKeys.thumbnailRenderer) as? PlaylistThumbnailRenderer }
        set { objc_setAssociatedObject(self, &AssociatedKeys.thumbnailRenderer, newValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
    }
}
