// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveShared
import Shared
import AVKit
import AVFoundation
import SDWebImage
import CoreData
import Data
import Combine

private let log = Logger.browserLogger

// MARK: - DisplayMode

private enum DisplayMode {
    case iPhoneLayout
    case iPadLayout
}

// MARK: PlaylistViewController

class PlaylistViewController: UIViewController {
    
    // MARK: Properties

    private let splitController = UISplitViewController()
    private let listController = ListController()
    private let detailController = DetailController()
    
    init(initialItem: PlaylistInfo?, initialItemPlaybackOffset: Double) {
        super.init(nibName: nil, bundle: nil)
        
        listController.initialItem = initialItem
        listController.initialItemPlaybackOffset = initialItemPlaybackOffset
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        overrideUserInterfaceStyle = .dark
        
        splitController.do {
            $0.viewControllers = [SettingsNavigationController(rootViewController: listController),
                                  SettingsNavigationController(rootViewController: detailController)]
            $0.delegate = self
            $0.primaryEdge = PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? .leading : .trailing
            $0.presentsWithGesture = false
            $0.maximumPrimaryColumnWidth = 400
            $0.minimumPrimaryColumnWidth = 400
        }
        
        addChild(splitController)
        view.addSubview(splitController.view)
        
        splitController.do {
            $0.didMove(toParent: self)
            $0.view.translatesAutoresizingMaskIntoConstraints = false
            $0.view.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
        }
        
        updateLayoutForOrientationChange()
        
        detailController.setVideoPlayer(listController.playerView)
        detailController.navigationController?.setNavigationBarHidden(splitController.isCollapsed || traitCollection.horizontalSizeClass == .regular, animated: false)
        
        if UIDevice.isPhone {
            if splitController.isCollapsed == false && traitCollection.horizontalSizeClass == .regular {
                listController.updateLayoutForMode(.iPadLayout)
                detailController.updateLayoutForMode(.iPadLayout)
            } else {
                listController.updateLayoutForMode(.iPhoneLayout)
                detailController.updateLayoutForMode(.iPhoneLayout)
                
                // On iPhone Pro Max which displays like an iPad, we need to hide navigation bar.
                if UIDevice.isPhone && UIDevice.current.orientation.isLandscape {
                    listController.onFullScreen()
                }
            }
        } else {
            listController.updateLayoutForMode(.iPadLayout)
            detailController.updateLayoutForMode(.iPadLayout)
        }
    }
    
    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        
        updateLayoutForOrientationChange()
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }
    
    private func updateLayoutForOrientationChange() {
        if listController.playerView.isFullscreen {
            splitController.preferredDisplayMode = .secondaryOnly
        } else {
            if UIDevice.current.orientation.isLandscape {
                splitController.preferredDisplayMode = .secondaryOnly
            } else {
                splitController.preferredDisplayMode = .primaryOverlay
            }
        }
    }
    
    fileprivate func onSidePanelStateChanged() {
        detailController.onSidePanelStateChanged()
    }
    
    fileprivate func onFullscreen() {
        detailController.onFullScreen()
    }
    
    fileprivate func onExitFullscreen() {
        detailController.onExitFullScreen()
    }
}

// MARK: - UIAdaptivePresentationControllerDelegate

extension PlaylistViewController: UIAdaptivePresentationControllerDelegate {
    func adaptivePresentationStyle(for controller: UIPresentationController, traitCollection: UITraitCollection) -> UIModalPresentationStyle {
        return .fullScreen
    }
}

// MARK: - UISplitViewControllerDelegate

extension PlaylistViewController: UISplitViewControllerDelegate {
    func splitViewControllerSupportedInterfaceOrientations(_ splitViewController: UISplitViewController) -> UIInterfaceOrientationMask {
        return .allButUpsideDown
    }
    
    func splitViewController(_ splitViewController: UISplitViewController, collapseSecondary secondaryViewController: UIViewController, onto primaryViewController: UIViewController) -> Bool {
        
        // On iPhone, always display the iPhone layout (collapsed) no matter what.
        // On iPad, we need to update both the list controller's layout (collapsed) and the detail controller's layout (collapsed).
        listController.updateLayoutForMode(.iPhoneLayout)
        detailController.setVideoPlayer(nil)
        detailController.updateLayoutForMode(.iPhoneLayout)
        return true
    }
    
    func splitViewController(_ splitViewController: UISplitViewController, separateSecondaryFrom primaryViewController: UIViewController) -> UIViewController? {
        
        // On iPhone, always display the iPad layout (expanded) when not in compact mode.
        // On iPad, we need to update both the list controller's layout (expanded) and the detail controller's layout (expanded).
        listController.updateLayoutForMode(.iPadLayout)
        detailController.setVideoPlayer(listController.playerView)
        detailController.updateLayoutForMode(.iPadLayout)
        
        if UIDevice.isPhone {
            detailController.navigationController?.setNavigationBarHidden(true, animated: true)
        }
        
        return detailController.navigationController ?? detailController
    }
}

// MARK: - ListController

private class ListController: UIViewController {
    // MARK: Constants
     
     struct Constants {
        static let playListCellIdentifier = "playlistCellIdentifier"
        static let tableRowHeight: CGFloat = 80
        static let tableHeaderHeight: CGFloat = 11
     }

    // MARK: Properties
    public var initialItem: PlaylistInfo?
    public var initialItemPlaybackOffset = 0.0
    
    public let playerView = VideoView()
    private lazy var mediaInfo = PlaylistMediaInfo(playerView: playerView)
    private var currentlyPlayingItemIndex = -1
    private var autoPlayEnabled = true
    private var playerController: AVPlayerViewController?
    private var playerStatusObserver: PlaylistPlayerStatusObserver?
    
    private lazy var activityIndicator = UIActivityIndicatorView(style: .medium).then {
        $0.isHidden = true
        $0.hidesWhenStopped = true
    }
    
    private let tableView = UITableView(frame: .zero, style: .grouped).then {
        $0.backgroundView = UIView()
        $0.backgroundColor = .braveBackground
        $0.separatorColor = .clear
        $0.allowsSelectionDuringEditing = true
    }
    
    private let formatter = DateComponentsFormatter().then {
        $0.allowedUnits = [.day, .hour, .minute, .second]
        $0.unitsStyle = .abbreviated
        $0.maximumUnitCount = 2
    }
    
    init() {
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    deinit {
        if let playTime = playerView.player.currentItem?.currentTime(),
           Preferences.Playlist.playbackLeftOff.value {
            Preferences.Playlist.lastPlayedItemTime.value = playTime.seconds
        } else {
            Preferences.Playlist.lastPlayedItemTime.value = 0.0
        }
        
        playerStatusObserver = nil
        playerView.pictureInPictureController?.delegate = nil
        playerView.pictureInPictureController?.stopPictureInPicture()
        playerView.stop()
        
        NotificationCenter.default.removeObserver(self)
        
        if let delegate = UIApplication.shared.delegate as? AppDelegate {
            if UIDevice.isIpad {
                playerView.attachLayer()
            }
            delegate.playlistRestorationController = nil
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        PlaylistManager.shared.delegate = self
    
        setTheme()
        setup()
        setupNotifications()

        fetchResults()
    }
    
    // MARK: Internal
    
    private func setTheme() {
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
    }
    
    private func setup () {
        tableView.do {
            $0.register(PlaylistCell.self, forCellReuseIdentifier: Constants.playListCellIdentifier)
            $0.dataSource = self
            $0.delegate = self
            $0.dragDelegate = self
            $0.dropDelegate = self
            $0.dragInteractionEnabled = true
        }
        
        playerView.delegate = self
    }
    
    private func setupNotifications() {
        // Instead of observing every single second the player's time changes,
        // we observe when the user terminates the app to update the last playing time.
        // We do the same in the deinit of this controller.
        // This is necessary because `deinit` isn't called on termination and it would be an insane
        // performance issue to write to preferences every time the player's time changes.
        
        let updatedAppStatuses = [
            UIApplication.willResignActiveNotification,
            UIApplication.willTerminateNotification,
            UIApplication.didEnterBackgroundNotification
        ]
        
        updatedAppStatuses.forEach({
            NotificationCenter.default.addObserver(self, selector: #selector(updateLastPlayedItemTime(_:)), name: $0, object: nil)
        })
    }
    
    @objc
    func updateLastPlayedItemTime(_ notification: Notification) {
        if let playTime = playerView.player.currentItem?.currentTime(),
           Preferences.Playlist.playbackLeftOff.value {
            Preferences.Playlist.lastPlayedItemTime.value = playTime.seconds
        } else {
            Preferences.Playlist.lastPlayedItemTime.value = 0.0
        }
    }
    
    private func fetchResults() {
        playerView.setControlsEnabled(playerView.player.currentItem != nil)
        updateTableBackgroundView()
        
        let initialItem = self.initialItem
        let initialItemOffset = self.initialItemPlaybackOffset
        self.initialItem = nil
        self.initialItemPlaybackOffset = 0.0
        
        DispatchQueue.main.async {
            PlaylistManager.shared.reloadData()
            self.tableView.reloadData()
            
            let lastPlayedItemUrl = initialItem?.pageSrc ?? Preferences.Playlist.lastPlayedItemUrl.value
            let lastPlayedItemTime = initialItem != nil ? initialItemOffset : Preferences.Playlist.lastPlayedItemTime.value
            
            self.autoPlayEnabled = initialItem != nil ? true : Preferences.Playlist.firstLoadAutoPlay.value
            
            if PlaylistManager.shared.numberOfAssets > 0 {
                self.playerView.setControlsEnabled(true)
                
                if let lastPlayedItemUrl = lastPlayedItemUrl, let index = PlaylistManager.shared.index(of: lastPlayedItemUrl) {
                    let indexPath = IndexPath(row: index, section: 0)
                    
                    self.playItem(at: indexPath, completion: { [weak self] error in
                        guard let self = self else { return }
                        
                        switch error {
                        case .error(let err):
                            log.error(err)
                            self.displayLoadingResourceError()
                        case .expired:
                            let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
                            self.displayExpiredResourceError(item: item)
                        case .none:
                            let seekLastPlayedItem = { [weak self] in
                                guard let self = self else { return }
                                let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
                                
                                if item.pageSrc == lastPlayedItemUrl &&
                                    lastPlayedItemTime > 0.0 &&
                                    Preferences.Playlist.playbackLeftOff.value {
                                    self.playerView.seek(to: lastPlayedItemTime)
                                }
                                
                                self.updateLastPlayedItem(indexPath: indexPath)
                            }
                            
                            if self.playerView.player.status == .readyToPlay {
                                seekLastPlayedItem()
                            } else {
                                self.playerStatusObserver = PlaylistPlayerStatusObserver(player: self.playerView.player, onStatusChanged: { [weak self] status in
                                    guard let self = self else { return }
                                    
                                    log.debug("Player Status: \(status)")
                                    
                                    seekLastPlayedItem()
                                    self.playerStatusObserver = nil
                                })
                            }
                        }
                    })
                } else {
                    self.tableView.delegate?.tableView?(self.tableView, didSelectRowAt: IndexPath(row: 0, section: 0))
                }
                
                self.autoPlayEnabled = true
            }
            
            self.updateTableBackgroundView()
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
    
    public func updateLayoutForMode(_ mode: DisplayMode) {
        navigationItem.rightBarButtonItem = nil
        
        if mode == .iPhoneLayout {
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
            updateLayoutForMode(.iPhoneLayout)
            
            if !playerView.isFullscreen {
                navigationController?.setNavigationBarHidden(UIDevice.current.orientation.isLandscape, animated: true)
            }
        }
    }
}

// MARK: UITableViewDataSource

extension ListController: UITableViewDataSource {
    private func getRelativeDateFormat(date: Date) -> String {
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .abbreviated
        formatter.dateTimeStyle = .numeric
        return formatter.localizedString(fromTimeInterval: date.timeIntervalSinceNow)
    }
    
    private func getAssetDuration(item: PlaylistInfo, _ completion: @escaping (TimeInterval?, AVAsset?) -> Void) -> PlaylistAssetFetcher? {
        let tolerance: Double = 0.00001
        let distance = abs(item.duration.distance(to: 0.0))
        
        // If the database duration is live/indefinite
        if item.duration.isInfinite ||
            abs(item.duration.distance(to: TimeInterval.greatestFiniteMagnitude)) < tolerance {
            completion(TimeInterval.infinity, nil)
            return nil
        }
        
        // If the database duration is 0.0
        if distance >= tolerance {
            // Return the database duration
            completion(item.duration, nil)
            return nil
        }
        
        guard let index = PlaylistManager.shared.index(of: item.pageSrc) else {
            completion(item.duration, nil) // Return the database duration
            return nil
        }
        
        // Attempt to retrieve the duration from the Asset file
        let asset = PlaylistManager.shared.assetAtIndex(index)
        
        // Accessing tracks blocks the main-thread if not already loaded
        // So we first need to check the track status before attempting to access it!
        var error: NSError?
        let trackStatus = asset.statusOfValue(forKey: "tracks", error: &error)
        if let error = error {
            log.error("AVAsset.statusOfValue error occurred: \(error)")
        }
        
        if trackStatus == .loaded {
            if !asset.tracks.isEmpty,
               let track = asset.tracks(withMediaType: .video).first ??
                            asset.tracks(withMediaType: .audio).first {
                if track.timeRange.duration.isIndefinite {
                    completion(TimeInterval.infinity, nil)
                } else {
                    completion(track.timeRange.duration.seconds, asset)
                }
                return nil
            }
        } else {
            log.debug("AVAsset.statusOfValue not loaded. Status: \(trackStatus)")
        }
        
        // Accessing duration or commonMetadata blocks the main-thread if not already loaded
        // So we first need to check the track status before attempting to access it!
        let durationStatus = asset.statusOfValue(forKey: "duration", error: &error)
        if let error = error {
            log.error("AVAsset.statusOfValue error occurred: \(error)")
        }
        
        if durationStatus == .loaded {
            // If it's live/indefinite
            if asset.duration.isIndefinite {
                completion(TimeInterval.infinity, asset)
                return nil
            }
            
            // If it's a valid duration
            if abs(asset.duration.seconds.distance(to: 0.0)) >= tolerance {
                completion(asset.duration.seconds, asset)
                return nil
            }
        } else {
            log.debug("AVAsset.statusOfValue not loaded. Status: \(durationStatus)")
        }
        
        // We can't get the duration synchronously so we need to let the AVAsset load the media item
        // and hopefully we get a valid duration from that.
        asset.loadValuesAsynchronously(forKeys: ["playable", "tracks", "duration"]) {
            var error: NSError?
            let trackStatus = asset.statusOfValue(forKey: "tracks", error: &error)
            if let error = error {
                log.error("AVAsset.statusOfValue error occurred: \(error)")
            }
            
            let durationStatus = asset.statusOfValue(forKey: "tracks", error: &error)
            if let error = error {
                log.error("AVAsset.statusOfValue error occurred: \(error)")
            }
            
            if trackStatus == .cancelled || durationStatus == .cancelled {
                return
            }
            
            if trackStatus == .failed && durationStatus == .failed, let error = error {
                if error.code == NSURLErrorNoPermissionsToReadFile {
                    // Media item is expired.. permission is denied
                    log.debug("Playlist Media Item Expired: \(item.pageSrc)")
                    
                    ensureMainThread {
                        completion(nil, nil)
                    }
                } else {
                    log.error("An unknown error occurred while attempting to fetch track and duration information: \(error)")
                    
                    ensureMainThread {
                        completion(nil, nil)
                    }
                }
                
                return
            }
            
            var duration: CMTime = .zero
            if trackStatus == .loaded {
                if let track = asset.tracks(withMediaType: .video).first ?? asset.tracks(withMediaType: .audio).first {
                    duration = track.timeRange.duration
                } else {
                    duration = asset.duration
                }
            } else if durationStatus == .loaded {
                duration = asset.duration
            }

            ensureMainThread {
                if duration.isIndefinite {
                    completion(TimeInterval.infinity, asset)
                } else if abs(duration.seconds.distance(to: 0.0)) > tolerance {
                    let newItem = PlaylistInfo(name: item.name,
                                               src: item.src,
                                               pageSrc: item.pageSrc,
                                               pageTitle: item.pageTitle,
                                               mimeType: item.mimeType,
                                               duration: duration.seconds,
                                               detected: item.detected,
                                               dateAdded: item.dateAdded,
                                               tagId: item.tagId)

                    PlaylistItem.updateItem(newItem) {
                        completion(duration.seconds, asset)
                    }
                } else {
                    completion(duration.seconds, asset)
                }
            }
        }
        
        return PlaylistAssetFetcher(asset: asset)
    }
    
    private func getAssetDurationFormatted(item: PlaylistInfo, _ completion: @escaping (String) -> Void) -> PlaylistAssetFetcher? {
        return getAssetDuration(item: item) { [weak self] duration, asset in
            guard let self = self else { return }
            
            let domain = URL(string: item.pageSrc)?.baseDomain ?? "0s"
            if let duration = duration {
                if duration.isInfinite {
                    // Live video/audio
                    completion(Strings.PlayList.playlistLiveMediaStream)
                } else if abs(duration.distance(to: 0.0)) > 0.00001 {
                    completion(self.formatter.string(from: duration) ?? domain)
                } else {
                    completion(domain)
                }
            } else {
                // Media Item is expired or some sort of error occurred retrieving its duration
                // Whatever the reason, we mark it as expired now
                completion(Strings.PlayList.expiredLabelTitle)
            }
        }
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        PlaylistManager.shared.numberOfAssets
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        Constants.tableRowHeight
    }
    
    func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        Constants.tableHeaderHeight
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let cell = tableView.dequeueReusableCell(withIdentifier: Constants.playListCellIdentifier, for: indexPath) as? PlaylistCell else {
            return UITableViewCell()
        }
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
        guard let cell = cell as? PlaylistCell else {
            return
        }
        
        cell.prepareForDisplay()
        let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
        let domain = URL(string: item.pageSrc)?.baseDomain ?? "0s"
        
        cell.do {
            $0.selectionStyle = .none
            $0.titleLabel.text = item.name
            $0.detailLabel.text = domain
            $0.contentView.backgroundColor = .clear
            $0.backgroundColor = .clear
            $0.thumbnailView.image = nil
            $0.thumbnailView.backgroundColor = .black
        }
        
        let cacheState = PlaylistManager.shared.state(for: item.pageSrc)
        switch cacheState {
        case .inProgress:
            cell.durationFetcher = getAssetDurationFormatted(item: item) {
                cell.detailLabel.text = "\($0) - \(Strings.PlayList.savingForOfflineLabelTitle)"
            }
        case .downloaded:
            if let itemSize = PlaylistManager.shared.sizeOfDownloadedItem(for: item.pageSrc) {
                cell.durationFetcher = getAssetDurationFormatted(item: item) {
                    cell.detailLabel.text = "\($0) - \(itemSize)"
                }
            } else {
                cell.durationFetcher = getAssetDurationFormatted(item: item) {
                    cell.detailLabel.text = "\($0) - \(Strings.PlayList.savedForOfflineLabelTitle)"
                }
            }
        case .invalid:
            cell.durationFetcher = getAssetDurationFormatted(item: item) {
                cell.detailLabel.text = $0
            }
        }
        
        // Load the HLS/Media thumbnail. If it fails, fall-back to favIcon
        loadThumbnail(item: item, cell: cell)
    }
    
    func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let headerView = UIView()
        headerView.backgroundColor = .clear
        
        return headerView
    }
    
    // MARK: - Thumbnail
    
    private func loadThumbnail(item: PlaylistInfo, cell: PlaylistCell) {
        guard let url = URL(string: item.src) else { return }
        
        cell.thumbnailActivityIndicator.startAnimating()
        if let cachedImage = SDImageCache.shared.imageFromCache(forKey: url.absoluteString) {
            cell.thumbnailView.image = cachedImage
            cell.thumbnailView.backgroundColor = .black
            cell.thumbnailView.contentMode = .scaleAspectFit
            cell.thumbnailActivityIndicator.stopAnimating()
            cell.thumbnailGenerator = nil
            return
        }
        
        // Loading from Cache failed, attempt to fetch HLS thumbnail
        cell.thumbnailActivityIndicator.startAnimating()
        cell.thumbnailGenerator = HLSThumbnailGenerator(url: url, time: 3, completion: { [weak self, weak cell] image, error in
            guard let self = self, let cell = cell else { return }
            
            cell.thumbnailGenerator = nil
            cell.thumbnailView.stopAnimating()
            log.error(error)
            
            if let image = image {
                cell.thumbnailView.image = image
                cell.thumbnailView.backgroundColor = .black
                cell.thumbnailView.contentMode = .scaleAspectFit
                cell.thumbnailGenerator = nil
                SDImageCache.shared.store(image, forKey: url.absoluteString, completion: nil)
            } else {
                // We can fall back to AVAssetImageGenerator or FavIcon
                self.loadThumbnailFallbackImage(item: item, cell: cell)
            }
        })
    }
    
    // Fall back to AVAssetImageGenerator
    // If that fails, fallback to FavIconFetcher
    private func loadThumbnailFallbackImage(item: PlaylistInfo, cell: PlaylistCell) {
        guard let url = URL(string: item.src) else { return }

        let time = CMTimeMake(value: 3, timescale: 1)
        cell.thumbnailActivityIndicator.startAnimating()
        cell.imageAssetGenerator = AVAssetImageGenerator(asset: AVAsset(url: url))
        cell.imageAssetGenerator?.appliesPreferredTrackTransform = false
        cell.imageAssetGenerator?.generateCGImagesAsynchronously(forTimes: [NSValue(time: time)]) { [weak cell] _, cgImage, _, result, error in
            guard let cell = cell else { return }
            
            cell.imageAssetGenerator = nil
            if result == .succeeded, let cgImage = cgImage {
                let image = UIImage(cgImage: cgImage)

                DispatchQueue.main.async {
                    cell.thumbnailActivityIndicator.stopAnimating()
                    cell.thumbnailView.image = image
                    cell.thumbnailView.backgroundColor = .black
                    cell.thumbnailView.contentMode = .scaleAspectFit
                    SDImageCache.shared.store(image, forKey: url.absoluteString, completion: nil)
                }
            } else {
                guard let url = URL(string: item.pageSrc) else { return }
                
                DispatchQueue.main.async {
                    cell.thumbnailActivityIndicator.stopAnimating()
                    cell.thumbnailView.cancelFaviconLoad()
                    cell.thumbnailView.clearMonogramFavicon()
                    cell.thumbnailView.contentMode = .scaleAspectFit
                    cell.thumbnailView.image = FaviconFetcher.defaultFaviconImage
                    cell.thumbnailActivityIndicator.startAnimating()
                    
                    let domain = Domain.getOrCreate(forUrl: url, persistent: false)
                    cell.thumbnailView.loadFavicon(for: url, domain: domain) { [weak cell] in
                        guard let cell = cell else { return }
                        cell.thumbnailActivityIndicator.stopAnimating()
                    }
                }
            }
        }
    }
}

// MARK: UITableViewDelegate

extension ListController: UITableViewDelegate {
    
    func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
        
        if indexPath.row < 0 || indexPath.row >= PlaylistManager.shared.numberOfAssets {
            return nil
        }

        let currentItem = PlaylistManager.shared.itemAtIndex(indexPath.row)
        let cacheState = PlaylistManager.shared.state(for: currentItem.pageSrc)
        
        let cacheAction = UIContextualAction(style: .normal, title: nil, handler: { [weak self] (action, view, completionHandler) in
            guard let self = self else { return }
            
            switch cacheState {
                case .inProgress:
                    PlaylistManager.shared.cancelDownload(item: currentItem)
                    self.tableView.reloadRows(at: [indexPath], with: .automatic)
                case .invalid:
                    if PlaylistManager.shared.isDiskSpaceEncumbered() {
                        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
                        let alert = UIAlertController(
                            title: Strings.PlayList.playlistDiskSpaceWarningTitle, message: Strings.PlayList.playlistDiskSpaceWarningMessage, preferredStyle: style)
                        
                        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: { _ in
                            PlaylistManager.shared.download(item: currentItem)
                            self.tableView.reloadRows(at: [indexPath], with: .automatic)
                        }))
                        
                        alert.addAction(UIAlertAction(title: Strings.CancelString, style: .cancel, handler: nil))
                        self.present(alert, animated: true, completion: nil)
                    } else {
                        PlaylistManager.shared.download(item: currentItem)
                        self.tableView.reloadRows(at: [indexPath], with: .automatic)
                    }
                case .downloaded:
                    let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
                    let alert = UIAlertController(
                        title: Strings.PlayList.removePlaylistOfflineDataAlertTitle, message: Strings.PlayList.removePlaylistOfflineDataAlertMessage, preferredStyle: style)
                    
                    alert.addAction(UIAlertAction(title: Strings.PlayList.removeActionButtonTitle, style: .destructive, handler: { _ in
                        PlaylistManager.shared.deleteCache(item: currentItem)
                        self.tableView.reloadRows(at: [indexPath], with: .automatic)
                    }))
                    
                    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
                    
                    self.present(alert, animated: true, completion: nil)
            }
            
            completionHandler(true)
        })
        
        let deleteAction = UIContextualAction(style: .normal, title: nil, handler: { [weak self] (action, view, completionHandler) in
            guard let self = self else { return }
            
            let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
            let alert = UIAlertController(
                title: Strings.PlayList.removePlaylistVideoAlertTitle, message: Strings.PlayList.removePlaylistVideoAlertMessage, preferredStyle: style)
            
            alert.addAction(UIAlertAction(title: Strings.PlayList.removeActionButtonTitle, style: .destructive, handler: { _ in
                PlaylistManager.shared.delete(item: currentItem)

                if self.currentlyPlayingItemIndex == indexPath.row {
                    self.currentlyPlayingItemIndex = -1
                    self.mediaInfo.nowPlayingInfo = nil
                    self.mediaInfo.updateNowPlayingMediaArtwork(image: nil)
                    
                    self.updateTableBackgroundView()
                    self.playerView.resetVideoInfo()
                    self.activityIndicator.stopAnimating()
                    self.playerView.stop()
                }
            }))
            
            alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
            self.present(alert, animated: true, completion: nil)
            
            completionHandler(true)
        })

        cacheAction.image = cacheState == .invalid ? #imageLiteral(resourceName: "playlist_download") : #imageLiteral(resourceName: "playlist_delete_download")
        cacheAction.backgroundColor = #colorLiteral(red: 0.4509803922, green: 0.4784313725, blue: 0.8705882353, alpha: 1)
        
        deleteAction.image = #imageLiteral(resourceName: "playlist_delete_item")
        deleteAction.backgroundColor = #colorLiteral(red: 0.9176470588, green: 0.2274509804, blue: 0.05098039216, alpha: 1)
        
        return UISwipeActionsConfiguration(actions: [deleteAction, cacheAction])
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if tableView.isEditing {
            tableView.setEditing(false, animated: true)
            return
        }
        
        playItem(at: indexPath, completion: { [weak self] error in
            guard let self = self else { return }
            
            switch error {
            case .error(let err):
                log.error(err)
                self.displayLoadingResourceError()
            case .expired:
                let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
                self.displayExpiredResourceError(item: item)
            case .none:
                self.updateLastPlayedItem(indexPath: indexPath)
            }
        })
    }
    
    private func playItem(at indexPath: IndexPath, completion: ((PlaylistMediaInfo.MediaPlaybackError) -> Void)?) {
        if indexPath.row >= PlaylistManager.shared.numberOfAssets {
            return
        }
        
        activityIndicator.startAnimating()
        activityIndicator.isHidden = false
        currentlyPlayingItemIndex = indexPath.row
        
        let selectedCell = tableView.cellForRow(at: indexPath) as? PlaylistCell

        let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
        playerView.setVideoInfo(videoDomain: item.pageSrc, videoTitle: item.pageTitle)
        mediaInfo.updateNowPlayingMediaArtwork(image: selectedCell?.thumbnailView.image)
        
        playerView.stop()
        
        mediaInfo.loadMediaItem(item, index: indexPath.row, autoPlayEnabled: autoPlayEnabled) { [weak self] error in
            guard let self = self else { return }
            defer { completion?(error) }
            self.activityIndicator.stopAnimating()
            
            switch error {
            case .error:
                break
                
            case .expired:
                selectedCell?.detailLabel.text = Strings.PlayList.expiredLabelTitle
                
            case .none:
                let mediaItem = self.playerView.player.currentItem ?? self.playerView.pendingMediaItem
                log.debug("Playing Live Video: \(mediaItem?.duration.isIndefinite ?? false)")
            }
        }
    }
    
    private func updateLastPlayedItem(indexPath: IndexPath) {
        let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
        Preferences.Playlist.lastPlayedItemUrl.value = item.pageSrc
        
        if let playTime = self.playerView.player.currentItem?.currentTime(),
           Preferences.Playlist.playbackLeftOff.value {
            Preferences.Playlist.lastPlayedItemTime.value = playTime.seconds
        } else {
            Preferences.Playlist.lastPlayedItemTime.value = 0.0
        }
    }
    
    private func displayExpiredResourceError(item: PlaylistInfo) {
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
    }
    
    private func displayLoadingResourceError() {
        let alert = UIAlertController(
            title: Strings.PlayList.sorryAlertTitle, message: Strings.PlayList.loadResourcesErrorAlertDescription, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
        
        self.present(alert, animated: true, completion: nil)
    }
}

// MARK: - Reordering of cells

extension ListController: UITableViewDragDelegate, UITableViewDropDelegate {
    func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
        true
    }
    
    func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
        .none
    }
    
    func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
        false
    }
    
    func tableView(_ tableView: UITableView, moveRowAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        PlaylistManager.shared.reorderItems(from: sourceIndexPath, to: destinationIndexPath) {
            PlaylistManager.shared.reloadData()
        }
    }
    
    func tableView(_ tableView: UITableView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
        let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
        let dragItem = UIDragItem(itemProvider: NSItemProvider())
        dragItem.localObject = item
        return [dragItem]
    }
    
    func tableView(_ tableView: UITableView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UITableViewDropProposal {
        
        var dropProposal = UITableViewDropProposal(operation: .cancel)
        guard session.items.count == 1 else { return dropProposal }
        
        if tableView.hasActiveDrag {
            dropProposal = UITableViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
        }
        return dropProposal
    }
        
    func tableView(_ tableView: UITableView, performDropWith coordinator: UITableViewDropCoordinator) {
        guard let sourceIndexPath = coordinator.items.first?.sourceIndexPath else { return }
        let destinationIndexPath: IndexPath
        if let indexPath = coordinator.destinationIndexPath {
            destinationIndexPath = indexPath
        } else {
            let section = tableView.numberOfSections - 1
            let row = tableView.numberOfRows(inSection: section)
            destinationIndexPath = IndexPath(row: row, section: section)
        }
        
        if coordinator.proposal.operation == .move {
            guard let item = coordinator.items.first else { return }
            _ = coordinator.drop(item.dragItem, toRowAt: destinationIndexPath)
            tableView.moveRow(at: sourceIndexPath, to: destinationIndexPath)
        }
    }
    
    func tableView(_ tableView: UITableView, dragPreviewParametersForRowAt indexPath: IndexPath) -> UIDragPreviewParameters? {
        guard let cell = tableView.cellForRow(at: indexPath) as? PlaylistCell else { return nil }
        
        let preview = UIDragPreviewParameters()
        preview.visiblePath = UIBezierPath(roundedRect: cell.contentView.frame, cornerRadius: 12.0)
        preview.backgroundColor = slightlyLighterColour(color: UIColor.braveBackground)
        return preview
    }

    func tableView(_ tableView: UITableView, dropPreviewParametersForRowAt indexPath: IndexPath) -> UIDragPreviewParameters? {
        guard let cell = tableView.cellForRow(at: indexPath) as? PlaylistCell else { return nil }
        
        let preview = UIDragPreviewParameters()
        preview.visiblePath = UIBezierPath(roundedRect: cell.contentView.frame, cornerRadius: 12.0)
        preview.backgroundColor = slightlyLighterColour(color: UIColor.braveBackground)
        return preview
    }
    
    func tableView(_ tableView: UITableView, dragSessionIsRestrictedToDraggingApplication session: UIDragSession) -> Bool {
        true
    }
    
    private func slightlyLighterColour(color: UIColor) -> UIColor {
        let desaturation: CGFloat = 0.5
        var h: CGFloat = 0, s: CGFloat = 0
        var b: CGFloat = 0, a: CGFloat = 0

        guard color.getHue(&h, saturation: &s, brightness: &b, alpha: &a) else {return color}

        return UIColor(hue: h,
                       saturation: max(s - desaturation, 0.0),
                       brightness: b,
                       alpha: a)
    }
}

extension ListController: UIGestureRecognizerDelegate {
    func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
        return !tableView.isEditing
    }
}

// MARK: VideoViewDelegate

extension ListController: VideoViewDelegate {
    func onPreviousTrack(isUserInitiated: Bool) {
        if currentlyPlayingItemIndex <= 0 {
            return
        }
        
        let index = currentlyPlayingItemIndex - 1
        if index < PlaylistManager.shared.numberOfAssets {
            let indexPath = IndexPath(row: index, section: 0)
            playItem(at: indexPath) { [weak self] error in
                guard let self = self else { return }
                switch error {
                case .error(let err):
                    log.error(err)
                    self.displayLoadingResourceError()
                case .expired:
                    let item = PlaylistManager.shared.itemAtIndex(index)
                    self.displayExpiredResourceError(item: item)
                case .none:
                    self.currentlyPlayingItemIndex = index
                    self.updateLastPlayedItem(indexPath: indexPath)
                }
            }
        }
    }
    
    func onNextTrack(isUserInitiated: Bool) {
        let assetCount = PlaylistManager.shared.numberOfAssets
        let isAtEnd = currentlyPlayingItemIndex >= assetCount - 1
        var index = currentlyPlayingItemIndex
        
        switch playerView.repeatState {
        case .none:
            if isAtEnd {
                playerView.pictureInPictureController?.delegate = nil
                playerView.pictureInPictureController?.stopPictureInPicture()
                playerView.stop()
                
                if let delegate = UIApplication.shared.delegate as? AppDelegate {
                    if UIDevice.isIpad {
                        playerView.attachLayer()
                    }
                    delegate.playlistRestorationController = nil
                }
                return
            }
            index += 1
        case .repeatOne:
            playerView.seek(to: 0.0)
            playerView.play()
            return
        case .repeatAll:
            index = isAtEnd ? 0 : index + 1
        }
        
        if index >= 0 {
            let indexPath = IndexPath(row: index, section: 0)
            playItem(at: indexPath) { [weak self] error in
                guard let self = self else { return }
                switch error {
                case .error(let err):
                    log.error(err)
                    self.displayLoadingResourceError()
                case .expired:
                    if isUserInitiated || self.playerView.repeatState == .repeatOne || assetCount <= 1 {
                        let item = PlaylistManager.shared.itemAtIndex(index)
                        self.displayExpiredResourceError(item: item)
                    } else {
                        DispatchQueue.main.async {
                            self.currentlyPlayingItemIndex = index
                            self.onNextTrack(isUserInitiated: isUserInitiated)
                        }
                    }
                case .none:
                    self.currentlyPlayingItemIndex = index
                    self.updateLastPlayedItem(indexPath: indexPath)
                }
            }
        }
    }
    
    func onPictureInPicture(enabled: Bool) {
        playerView.pictureInPictureController?.delegate = enabled ? self : nil
    }
    
    func onSidePanelStateChanged() {
        (splitViewController?.parent as? PlaylistViewController)?.onSidePanelStateChanged()
    }
    
    func onFullScreen() {
        if !UIDevice.isIpad || splitViewController?.isCollapsed == true {
            navigationController?.setNavigationBarHidden(true, animated: true)
            tableView.isHidden = true
            playerView.snp.remakeConstraints {
                $0.edges.equalToSuperview()
            }
        } else {
            (splitViewController?.parent as? PlaylistViewController)?.onFullscreen()
        }
    }
    
    func onExitFullScreen() {
        if UIDevice.isIpad && splitViewController?.isCollapsed == false {
            playerView.setFullscreenButtonHidden(true)
            playerView.setExitButtonHidden(false)
            splitViewController?.parent?.dismiss(animated: true, completion: nil)
        } else if UIDevice.isIpad && splitViewController?.isCollapsed == true {
            navigationController?.setNavigationBarHidden(false, animated: true)
            playerView.setFullscreenButtonHidden(true)
            updateLayoutForMode(.iPhoneLayout)
        } else if UIDevice.current.orientation.isPortrait {
            navigationController?.setNavigationBarHidden(false, animated: true)
            tableView.isHidden = false
            updateLayoutForMode(.iPhoneLayout)
        } else {
            playerView.setFullscreenButtonHidden(true)
            playerView.setExitButtonHidden(false)
            splitViewController?.parent?.dismiss(animated: true, completion: nil)
        }
    }
}

// MARK: AVPlayerViewControllerDelegate && AVPictureInPictureControllerDelegate

extension ListController: AVPlayerViewControllerDelegate, AVPictureInPictureControllerDelegate {

    // MARK: - AVPlayerViewControllerDelegate
    
    func playerViewControllerShouldAutomaticallyDismissAtPictureInPictureStart(_ playerViewController: AVPlayerViewController) -> Bool {
        true
    }
    
    func playerViewController(_ playerViewController: AVPlayerViewController, willBeginFullScreenPresentationWithAnimationCoordinator coordinator: UIViewControllerTransitionCoordinator) {
        
        playerView.detachLayer()
        playerController = playerViewController
    }
    
    func playerViewController(_ playerViewController: AVPlayerViewController, willEndFullScreenPresentationWithAnimationCoordinator coordinator: UIViewControllerTransitionCoordinator) {
        
        playerView.attachLayer()
        playerController = nil
    }
    
    func playerViewControllerWillStartPictureInPicture(_ playerViewController: AVPlayerViewController) {
        playerView.detachLayer()
        
        (UIApplication.shared.delegate as? AppDelegate)?.playlistRestorationController = splitViewController?.parent
    }
    
    func playerViewControllerDidStartPictureInPicture(_ playerViewController: AVPlayerViewController) {
        DispatchQueue.main.async {
            self.playerView.detachLayer()
            self.dismiss(animated: true, completion: nil)
        }
    }
    
    func playerViewControllerDidStopPictureInPicture(_ playerViewController: AVPlayerViewController) {
        if let delegate = UIApplication.shared.delegate as? AppDelegate {
            playerView.attachLayer()
            delegate.playlistRestorationController = nil
            playerController = nil
        }
    }
    
    func playerViewController(_ playerViewController: AVPlayerViewController, failedToStartPictureInPictureWithError error: Error) {
        playerView.attachLayer()
        
        let alert = UIAlertController(title: Strings.PlayList.sorryAlertTitle,
                                      message: Strings.PlayList.pictureInPictureErrorTitle, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
        self.present(alert, animated: true, completion: nil)
    }
    
    func playerViewController(_ playerViewController: AVPlayerViewController, restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler: @escaping (Bool) -> Void) {
        
        if let delegate = UIApplication.shared.delegate as? AppDelegate,
           let restorationController = delegate.playlistRestorationController {
            restorationController.modalPresentationStyle = .fullScreen
            playerView.attachLayer()
            if view.window == nil {
                delegate.browserViewController.present(restorationController, animated: true) {
                    self.playerView.player.play()
                    delegate.playlistRestorationController = nil
                }
            } else {
                self.playerView.player.play()
                delegate.playlistRestorationController = nil
            }
        }
        
        playerController = nil
        completionHandler(true)
    }
    
    // MARK: - AVPictureInPictureControllerDelegate
    func pictureInPictureControllerWillStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
        
        (UIApplication.shared.delegate as? AppDelegate)?.playlistRestorationController = splitViewController?.parent
        
        if UIDevice.isIpad {
            splitViewController?.dismiss(animated: true, completion: nil)
        }
    }
    
    func pictureInPictureControllerDidStartPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
        if UIDevice.isPhone {
            DispatchQueue.main.async {
                self.dismiss(animated: true, completion: nil)
            }
        }
    }
    
    func pictureInPictureControllerDidStopPictureInPicture(_ pictureInPictureController: AVPictureInPictureController) {
        if let delegate = UIApplication.shared.delegate as? AppDelegate {
            if UIDevice.isIpad {
                playerView.attachLayer()
            }
            delegate.playlistRestorationController = nil
        }
    }
    
    func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, failedToStartPictureInPictureWithError error: Error) {
        
        let alert = UIAlertController(title: Strings.PlayList.sorryAlertTitle,
                                      message: Strings.PlayList.pictureInPictureErrorTitle, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil))
        self.present(alert, animated: true, completion: nil)
    }
    
    func pictureInPictureController(_ pictureInPictureController: AVPictureInPictureController, restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler: @escaping (Bool) -> Void) {
        
        if let delegate = UIApplication.shared.delegate as? AppDelegate,
           let restorationController = delegate.playlistRestorationController {
            restorationController.modalPresentationStyle = .fullScreen
            if view.window == nil {
                delegate.browserViewController.present(restorationController, animated: true) {
                    delegate.playlistRestorationController = nil
                }
            } else {
                delegate.playlistRestorationController = nil
            }
        }
        
        completionHandler(true)
    }
}

extension ListController: PlaylistManagerDelegate {
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
        
        let cacheState = PlaylistManager.shared.state(for: id)
        switch cacheState {
        case .inProgress:
            let item = PlaylistManager.shared.itemAtIndex(index)
            cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                cell?.detailLabel.text = "\($0) - \(Int(percentComplete))% \(Strings.PlayList.savedForOfflineLabelTitle)"
            }
        case .downloaded:
            let item = PlaylistManager.shared.itemAtIndex(index)
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
            let item = PlaylistManager.shared.itemAtIndex(index)
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
            
        if let error = error {
            log.error("Error downloading playlist item: \(error)")
            
            let item = PlaylistManager.shared.itemAtIndex(index)
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
                let item = PlaylistManager.shared.itemAtIndex(index)
                cell.durationFetcher = getAssetDurationFormatted(item: item) { [weak cell] in
                    cell?.detailLabel.text = "\($0) - \(Strings.PlayList.savingForOfflineLabelTitle)"
                }
            case .downloaded:
                let item = PlaylistManager.shared.itemAtIndex(index)
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
                let item = PlaylistManager.shared.itemAtIndex(index)
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

extension ListController {
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
}

private class DetailController: UIViewController, UIGestureRecognizerDelegate {
    
    private weak var playerView: VideoView?
    
    override func viewDidLoad() {
        super.viewDidLoad()

        setup()
        layoutBarButtons()
        addGestureRecognizers()
    }
    
    // MARK: Private
    
    private func setup() {
        view.backgroundColor = .black
                
        navigationController?.do {
            let appearance = UINavigationBarAppearance()
            appearance.configureWithTransparentBackground()
            appearance.titleTextAttributes = [.foregroundColor: UIColor.white]
            appearance.backgroundColor = .braveBackground
            
            $0.navigationBar.standardAppearance = appearance
            $0.navigationBar.barTintColor = UIColor.braveBackground
            $0.navigationBar.tintColor = .white
        }
    }
    
    private func layoutBarButtons() {
        let exitBarButton =  UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(onExit(_:)))
        let sideListBarButton = UIBarButtonItem(image: #imageLiteral(resourceName: "playlist_split_navigation"), style: .done, target: self, action: #selector(onDisplayModeChange))
        
        navigationItem.rightBarButtonItem =
            PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? exitBarButton : sideListBarButton
        navigationItem.leftBarButtonItem =
            PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? sideListBarButton : exitBarButton
    }
    
    private func addGestureRecognizers() {
        let slideToRevealGesture = UISwipeGestureRecognizer(target: self, action: #selector(handleGesture))
        slideToRevealGesture.direction = PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? .right : .left
        
        view.addGestureRecognizer(slideToRevealGesture)
    }
    
    private func updateSplitViewDisplayMode(to displayMode: UISplitViewController.DisplayMode) {
        UIView.animate(withDuration: 0.2) {
            self.splitViewController?.preferredDisplayMode = displayMode
        }
    }
    
    // MARK: Actions
    
    func onSidePanelStateChanged() {
        onDisplayModeChange()
    }
    
    func onFullScreen() {
        navigationController?.setNavigationBarHidden(true, animated: true)
        
        if navigationController?.isNavigationBarHidden == true {
            splitViewController?.preferredDisplayMode = .secondaryOnly
        }
    }
    
    func onExitFullScreen() {
        navigationController?.setNavigationBarHidden(false, animated: true)
        
        if navigationController?.isNavigationBarHidden == true {
            splitViewController?.preferredDisplayMode = .primaryOverlay
        }
    }
        
    @objc
    private func onExit(_ button: UIBarButtonItem) {
        dismiss(animated: true, completion: nil)
    }
    
    @objc
    func handleGesture(gesture: UISwipeGestureRecognizer) {
        guard gesture.direction == .right,
              let playerView = playerView,
              !playerView.checkInsideTrackBar(point: gesture.location(in: view)) else {
            return
        }
        
       onDisplayModeChange()
    }
    
    @objc
    private func onDisplayModeChange() {
        updateSplitViewDisplayMode(
            to: splitViewController?.displayMode == .primaryOverlay ? .secondaryOnly : .primaryOverlay)
    }
    
    public func setVideoPlayer(_ videoPlayer: VideoView?) {
        if playerView?.superview == view {
            playerView?.removeFromSuperview()
        }
        
        playerView = videoPlayer
    }
    
    public func updateLayoutForMode(_ mode: DisplayMode) {
        guard let playerView = playerView else { return }
        
        if mode == .iPadLayout {
            view.addSubview(playerView)
            playerView.snp.makeConstraints {
                $0.bottom.leading.trailing.equalTo(view)
                $0.top.equalTo(view.safeAreaLayoutGuide)
            }
        } else {
            if playerView.superview == view {
                playerView.removeFromSuperview()
            }
        }
    }
}
