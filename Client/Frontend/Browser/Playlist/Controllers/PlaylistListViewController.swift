// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import MediaPlayer
import AVFoundation
import AVKit
import CoreData
import Combine

// Third-Party
import SDWebImage

import BraveShared
import Shared
import Data

private let log = Logger.browserLogger

// MARK: - PlaylistListViewController

class PlaylistListViewController: UIViewController {
    // MARK: Constants
     
     struct Constants {
        static let playListCellIdentifier = "playlistCellIdentifier"
        static let tableRowHeight: CGFloat = 80
        static let tableHeaderHeight: CGFloat = 11
     }

    // MARK: Properties
    public var initialItem: PlaylistInfo?
    public var initialItemPlaybackOffset = 0.0
    
    weak var delegate: PlaylistViewControllerDelegate?
    private let playerView: VideoView
    private let contentManager = MPPlayableContentManager.shared()
    private var observers = Set<AnyCancellable>()
    private(set) var autoPlayEnabled = Preferences.Playlist.firstLoadAutoPlay.value
    var playerController: AVPlayerViewController?
    
    let activityIndicator = UIActivityIndicatorView(style: .medium).then {
        $0.color = .white
        $0.isHidden = true
        $0.hidesWhenStopped = true
    }
    
    let tableView = UITableView(frame: .zero, style: .grouped).then {
        $0.backgroundView = UIView()
        $0.backgroundColor = .braveBackground
        $0.separatorColor = .clear
        $0.allowsSelectionDuringEditing = true
    }
    
    init(playerView: VideoView) {
        self.playerView = playerView
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        PlaylistManager.shared.contentWillChange
        .sink { [weak self] in
            self?.controllerWillChangeContent()
        }.store(in: &observers)
        
        PlaylistManager.shared.contentDidChange
        .sink { [weak self] in
            self?.controllerDidChangeContent()
        }.store(in: &observers)
        
        PlaylistManager.shared.objectDidChange
        .sink { [weak self] in
            self?.controllerDidChange($0.object,
                                      at: $0.indexPath,
                                      for: $0.type,
                                      newIndexPath: $0.newIndexPath)
        }.store(in: &observers)
        
        PlaylistManager.shared.downloadProgressUpdated
        .sink { [weak self] in
            self?.onDownloadProgressUpdate(id: $0.id,
                                           percentComplete: $0.percentComplete)
        }.store(in: &observers)
        
        PlaylistManager.shared.downloadStateChanged
        .sink { [weak self] in
            self?.onDownloadStateChanged(id: $0.id,
                                         state: $0.state,
                                         displayName: $0.displayName,
                                         error: $0.error)
        }.store(in: &observers)
    
        // Theme
        title = Strings.PlayList.playListSectionTitle
        view.backgroundColor = .braveBackground
        navigationController?.do {
            let appearance = UINavigationBarAppearance()
            appearance.configureWithTransparentBackground()
            appearance.titleTextAttributes = [.foregroundColor: UIColor.white]
            appearance.backgroundColor = .braveBackground
            
            $0.navigationBar.standardAppearance = appearance
            $0.navigationBar.barTintColor = UIColor.braveBackground
            $0.navigationBar.tintColor = .white
        }
        
        // Layout
        tableView.do {
            $0.register(PlaylistCell.self, forCellReuseIdentifier: Constants.playListCellIdentifier)
            $0.dataSource = self
            $0.delegate = self
            $0.dragDelegate = self
            $0.dropDelegate = self
            $0.dragInteractionEnabled = true
        }

        // Update
        DispatchQueue.main.async {
            self.fetchResults()
        }
    }
    
    // MARK: Internal
    
    private func fetchResults() {
        updateTableBackgroundView()
        playerView.setControlsEnabled(false)
        
        let initialItem = self.initialItem
        let initialItemOffset = self.initialItemPlaybackOffset
        self.initialItem = nil
        self.initialItemPlaybackOffset = 0.0
        
        PlaylistManager.shared.reloadData()
        tableView.reloadData()
        contentManager.reloadData()
        
        // After reloading all data, update the background
        guard PlaylistManager.shared.numberOfAssets > 0 else {
            updateTableBackgroundView()
            autoPlayEnabled = true
            return
        }
        
        // Otherwise prepare to play the first item
        updateTableBackgroundView()
        playerView.setControlsEnabled(true)
        
        // If car play is active or media is already playing, do nothing
        if PlaylistCarplayManager.shared.isCarPlayAvailable && (delegate?.currentPlaylistAsset != nil || delegate?.isPlaying ?? false) {
            autoPlayEnabled = true
            return
        }
        
        // Setup initial playback item and time-offset
        let lastPlayedItemUrl = initialItem?.pageSrc ?? Preferences.Playlist.lastPlayedItemUrl.value
        let lastPlayedItemTime = initialItem != nil ? initialItemOffset : Preferences.Playlist.lastPlayedItemTime.value
        autoPlayEnabled = initialItem != nil ? true : Preferences.Playlist.firstLoadAutoPlay.value
        
        // If there is no last played item, then just select the first item in the playlist
        // which will play it if auto-play is enabled.
        guard let lastPlayedItemUrl = lastPlayedItemUrl,
              let index = PlaylistManager.shared.index(of: lastPlayedItemUrl) else {
            
            tableView.delegate?.tableView?(tableView, didSelectRowAt: IndexPath(row: 0, section: 0))
            autoPlayEnabled = true
            return
        }
        
        // Prepare the UI before playing the item
        let indexPath = IndexPath(row: index, section: 0)
        prepareToPlayItem(at: indexPath) { [weak self] item in
            guard let self = self,
                  let delegate = self.delegate,
                  let item = item else {
                self?.commitPlayerItemTransaction(at: indexPath,
                                                  isExpired: false)
                return
            }
            
            delegate.playItem(item: item) { [weak self] error in
                PlaylistCarplayManager.shared.currentPlaylistItem = nil
                
                guard let self = self,
                      let delegate = self.delegate else {
                    self?.commitPlayerItemTransaction(at: indexPath,
                                                     isExpired: false)
                    return
                }
                
                switch error {
                case .cancelled:
                    self.commitPlayerItemTransaction(at: indexPath,
                                                     isExpired: false)
                case .other(let err):
                    log.error(err)
                    self.commitPlayerItemTransaction(at: indexPath,
                                                     isExpired: false)
                    delegate.displayLoadingResourceError()
                case .expired:
                    self.commitPlayerItemTransaction(at: indexPath,
                                                     isExpired: true)
                    delegate.displayExpiredResourceError(item: item)
                case .none:
                    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
                    PlaylistCarplayManager.shared.currentPlaylistItem = item
                    self.commitPlayerItemTransaction(at: indexPath,
                                                     isExpired: false)
                    
                    // Update the player position/time-offset of the last played item
                    self.seekLastPlayedItem(at: indexPath,
                                            lastPlayedItemUrl: lastPlayedItemUrl,
                                            lastPlayedTime: lastPlayedItemTime)
                    
                    // Even if the item was NOT previously the last played item,
                    // it is now as it has begun to play
                    delegate.updateLastPlayedItem(item: item)
                }
            }
        }
        
        autoPlayEnabled = true
    }
    
    private func seekLastPlayedItem(at indexPath: IndexPath, lastPlayedItemUrl: String, lastPlayedTime: Double) {
        // The item can be deleted at any time,
        // so we need to guard against it and make sure the index path matches up correctly
        // If it does, we check the last played time
        // and seek to that position in the media item
        let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
        guard let item = item else { return }
        
        if item.pageSrc == lastPlayedItemUrl &&
            lastPlayedTime > 0.0 &&
            lastPlayedTime < delegate?.currentPlaylistAsset?.duration.seconds ?? 0.0 &&
            Preferences.Playlist.playbackLeftOff.value {
            self.playerView.seek(to: lastPlayedTime)
        }
    }
    
    // MARK: Actions
    
    @objc
    private func onExit(_ button: UIBarButtonItem) {
        dismiss(animated: true, completion: nil)
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        .lightContent
    }
    
    public func updateLayoutForMode(_ mode: UIUserInterfaceIdiom) {
        navigationItem.rightBarButtonItem = nil
        
        if mode == .phone {
            navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(onExit(_:)))
            
            playerView.setSidePanelHidden(true)
            
            // If the player view is in fullscreen, we should NOT change the tableView layout on rotation.
            view.addSubview(tableView)
            view.addSubview(playerView)
            playerView.addSubview(activityIndicator)
            
            if !playerView.isFullscreen {
                if UIDevice.current.orientation.isLandscape && UIDevice.isPhone {
                    playerView.setExitButtonHidden(false)
                    playerView.setFullscreenButtonHidden(true)
                    playerView.snp.remakeConstraints {
                        $0.edges.equalTo(view.snp.edges)
                    }
                    
                    activityIndicator.snp.remakeConstraints {
                        $0.center.equalToSuperview()
                    }
                } else {
                    playerView.setFullscreenButtonHidden(false)
                    playerView.setExitButtonHidden(true)
                    let videoPlayerHeight = (1.0 / 3.0) * (UIScreen.main.bounds.width > UIScreen.main.bounds.height ? UIScreen.main.bounds.width : UIScreen.main.bounds.height)

                    tableView.do {
                        $0.contentInset = UIEdgeInsets(top: videoPlayerHeight, left: 0.0, bottom: view.safeAreaInsets.bottom, right: 0.0)
                        $0.scrollIndicatorInsets = $0.contentInset
                        $0.contentOffset = CGPoint(x: 0.0, y: -videoPlayerHeight)
                        $0.isHidden = false
                    }
                    
                    playerView.snp.remakeConstraints {
                        $0.top.equalTo(view.safeArea.top)
                        $0.leading.trailing.equalToSuperview()
                        $0.height.equalTo(videoPlayerHeight)
                    }
                    
                    activityIndicator.snp.remakeConstraints {
                        $0.center.equalToSuperview()
                    }
                    
                    tableView.snp.remakeConstraints {
                        $0.edges.equalToSuperview()
                    }
                    
                    // On iPhone-8, 14.4, I need to scroll the tableView after setting its contentOffset and contentInset
                    // Otherwise the layout is broken when exiting fullscreen in portrait mode.
                    if PlaylistManager.shared.numberOfAssets > 0 {
                        tableView.scrollToRow(at: IndexPath(row: 0, section: 0), at: .top, animated: true)
                    }
                }
            } else {
                playerView.snp.remakeConstraints {
                    $0.edges.equalToSuperview()
                }
                
                activityIndicator.snp.remakeConstraints {
                    $0.center.equalToSuperview()
                }
            }
        } else {
            if splitViewController?.isCollapsed == true {
                playerView.setFullscreenButtonHidden(false)
                playerView.setExitButtonHidden(true)
                playerView.setSidePanelHidden(true)
            } else {
                playerView.setFullscreenButtonHidden(true)
                playerView.setExitButtonHidden(false)
                playerView.setSidePanelHidden(false)
            }
            
            view.addSubview(tableView)
            playerView.addSubview(activityIndicator)
            
            tableView.do {
                $0.contentInset = .zero
                $0.scrollIndicatorInsets = $0.contentInset
                $0.contentOffset = .zero
            }
            
            activityIndicator.snp.remakeConstraints {
                $0.center.equalToSuperview()
            }
            
            tableView.snp.remakeConstraints {
                $0.edges.equalToSuperview()
            }
        }
    }
    
    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)

        if UIDevice.isPhone && splitViewController?.isCollapsed == true {
            updateLayoutForMode(.phone)
            
            if !playerView.isFullscreen {
                navigationController?.setNavigationBarHidden(UIDevice.current.orientation.isLandscape, animated: true)
            }
        }
    }
}

extension PlaylistListViewController {
    func updateTableBackgroundView() {
        if PlaylistManager.shared.numberOfAssets > 0 {
            tableView.backgroundView = nil
            tableView.separatorStyle = .singleLine
        } else {
            let messageLabel = UILabel(frame: view.bounds).then {
                $0.text = Strings.PlayList.noItemLabelTitle
                $0.textColor = .white
                $0.numberOfLines = 0
                $0.textAlignment = .center
                $0.font = .systemFont(ofSize: 18.0, weight: .medium)
                $0.sizeToFit()
            }
            
            tableView.backgroundView = messageLabel
            tableView.separatorStyle = .none
        }
    }
    
    func prepareToPlayItem(at indexPath: IndexPath, _ completion: ((PlaylistInfo?) -> Void)?) {
        // Update the UI in preparation to play an item
        // Show the activity indicator, update the cell and player view, etc.
        guard indexPath.row < PlaylistManager.shared.numberOfAssets,
           let item = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
            completion?(nil)
            return
        }
        
        playerView.stop()
        playerView.bringSubviewToFront(activityIndicator)
        activityIndicator.startAnimating()
        activityIndicator.isHidden = false

        let selectedCell = tableView.cellForRow(at: indexPath) as? PlaylistCell
        playerView.setVideoInfo(videoDomain: item.pageSrc, videoTitle: item.pageTitle)
        PlaylistMediaStreamer.setNowPlayingMediaArtwork(image: selectedCell?.thumbnailView.image)
        completion?(item)
    }
    
    func commitPlayerItemTransaction(at indexPath: IndexPath, isExpired: Bool) {
        if isExpired {
            let selectedCell = tableView.cellForRow(at: indexPath) as? PlaylistCell
            selectedCell?.detailLabel.text = Strings.PlayList.expiredLabelTitle
        }
        
        activityIndicator.stopAnimating()
    }
}

// MARK: - Error Handling

extension PlaylistListViewController {
    func displayExpiredResourceError(item: PlaylistInfo?) {
        if let item = item {
            let alert = UIAlertController(title: Strings.PlayList.expiredAlertTitle,
                                          message: Strings.PlayList.expiredAlertDescription, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.PlayList.reopenButtonTitle, style: .default, handler: { _ in
                
                if let url = URL(string: item.pageSrc) {
                    self.dismiss(animated: true, completion: nil)
                    (UIApplication.shared.delegate as? AppDelegate)?.browserViewController.openURLInNewTab(url, isPrivileged: false)
                }
            }))
            alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
            self.present(alert, animated: true, completion: nil)
        } else {
            let alert = UIAlertController(title: Strings.PlayList.expiredAlertTitle,
                                          message: Strings.PlayList.expiredAlertDescription, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        }
    }
    
    func displayLoadingResourceError() {
        let alert = UIAlertController(
            title: Strings.PlayList.sorryAlertTitle, message: Strings.PlayList.loadResourcesErrorAlertDescription, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
        
        self.present(alert, animated: true, completion: nil)
    }
}

extension PlaylistListViewController: UIGestureRecognizerDelegate {
    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
        return !tableView.isEditing
    }
}

// MARK: VideoViewDelegate

extension PlaylistListViewController {
    
    func onFullscreen() {
        navigationController?.setNavigationBarHidden(true, animated: true)
        tableView.isHidden = true
        playerView.snp.remakeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    func onExitFullscreen() {
        if UIDevice.isIpad && splitViewController?.isCollapsed == false {
            playerView.setFullscreenButtonHidden(true)
            playerView.setExitButtonHidden(false)
            splitViewController?.parent?.dismiss(animated: true, completion: nil)
        } else if UIDevice.isIpad && splitViewController?.isCollapsed == true {
            navigationController?.setNavigationBarHidden(false, animated: true)
            playerView.setFullscreenButtonHidden(true)
            updateLayoutForMode(.phone)
        } else if UIDevice.current.orientation.isPortrait {
            navigationController?.setNavigationBarHidden(false, animated: true)
            tableView.isHidden = false
            updateLayoutForMode(.phone)
        } else {
            playerView.setFullscreenButtonHidden(true)
            playerView.setExitButtonHidden(false)
            splitViewController?.parent?.dismiss(animated: true, completion: nil)
        }
    }
    
    func onFavIconSelected(_ videoView: VideoView) {
        if let browser = PlaylistCarplayManager.shared.browserController,
           let currentItem = PlaylistCarplayManager.shared.currentPlaylistItem,
           let pageURL = URL(string: currentItem.pageSrc) {
            
            self.dismiss(animated: true) {
                let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
                browser.tabManager.addTabAndSelect(URLRequest(url: pageURL),
                                                   isPrivate: isPrivateBrowsing)
            }
        }
    }
}

// MARK: - PlaylistManagerDelegate

extension PlaylistListViewController: PlaylistManagerDelegate {
    func onDownloadProgressUpdate(id: String, percentComplete: Double) {
        guard let index = PlaylistManager.shared.index(of: id) else {
            return
        }
         
        let indexPath = IndexPath(row: index, section: 0)
        guard let cell = tableView.cellForRow(at: IndexPath(row: index, section: 0)) as? PlaylistCell else {
            return
        }
        
        // Cell is not visible, do not update percentages
        if tableView.indexPathsForVisibleRows?.contains(indexPath) == false {
            return
        }
        
        guard let item = PlaylistManager.shared.itemAtIndex(index) else {
            return
        }
        
        switch PlaylistManager.shared.state(for: id) {
        case .inProgress:
            cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                cell?.detailLabel.text = "\($0) - \(Int(percentComplete))% \(Strings.PlayList.savedForOfflineLabelTitle)"
            }
        case .downloaded:
            if let itemSize = PlaylistManager.shared.sizeOfDownloadedItem(for: item.pageSrc) {
                cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                    cell?.detailLabel.text = "\($0) - \(itemSize)"
                }
            } else {
                cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                    cell?.detailLabel.text = "\($0) - \(Strings.PlayList.savedForOfflineLabelTitle)"
                }
            }
        case .invalid:
            cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                cell?.detailLabel.text = $0
            }
        }
    }
    
    func onDownloadStateChanged(id: String, state: PlaylistDownloadManager.DownloadState, displayName: String?, error: Error?) {
        guard let index = PlaylistManager.shared.index(of: id) else {
            return
        }
         
        let indexPath = IndexPath(row: index, section: 0)
        guard let cell = tableView.cellForRow(at: IndexPath(row: index, section: 0)) as? PlaylistCell else {
            return
        }
        
        // Cell is not visible, do not update status
        if tableView.indexPathsForVisibleRows?.contains(indexPath) == false {
            return
        }
        
        guard let item = PlaylistManager.shared.itemAtIndex(index) else {
            return
        }
            
        if let error = error {
            log.error("Error downloading playlist item: \(error)")
            
            cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                cell?.detailLabel.text = $0
            }
            
            let alert = UIAlertController(title: Strings.PlayList.playlistSaveForOfflineErrorTitle,
                                          message: Strings.PlayList.playlistSaveForOfflineErrorMessage, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        } else {
            switch state {
            case .inProgress:
                cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                    cell?.detailLabel.text = "\($0) - \(Strings.PlayList.savingForOfflineLabelTitle)"
                }
            case .downloaded:
                if let itemSize = PlaylistManager.shared.sizeOfDownloadedItem(for: item.pageSrc) {
                    cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                        cell?.detailLabel.text = "\($0) - \(itemSize)"
                    }
                } else {
                    cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                        cell?.detailLabel.text = "\($0) - \(Strings.PlayList.savedForOfflineLabelTitle)"
                    }
                }
            case .invalid:
                cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                    cell?.detailLabel.text = $0
                }
            }
        }
    }
    
    func controllerDidChange(_ anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
        
        if tableView.hasActiveDrag || tableView.hasActiveDrop { return }
        
        switch type {
            case .insert:
                guard let newIndexPath = newIndexPath else { break }
                tableView.insertRows(at: [newIndexPath], with: .fade)
            case .delete:
                guard let indexPath = indexPath else { break }
                tableView.deleteRows(at: [indexPath], with: .fade)
            case .update:
                guard let indexPath = indexPath else { break }
                tableView.reloadRows(at: [indexPath], with: .fade)
            case .move:
                guard let indexPath = indexPath,
                      let newIndexPath = newIndexPath else { break }
                tableView.deleteRows(at: [indexPath], with: .fade)
                tableView.insertRows(at: [newIndexPath], with: .fade)
            default:
                break
        }
    }
    
    func controllerDidChangeContent() {
        if tableView.hasActiveDrag || tableView.hasActiveDrop { return }
        tableView.endUpdates()
    }
    
    func controllerWillChangeContent() {
        if tableView.hasActiveDrag || tableView.hasActiveDrop { return }
        tableView.beginUpdates()
    }
}
