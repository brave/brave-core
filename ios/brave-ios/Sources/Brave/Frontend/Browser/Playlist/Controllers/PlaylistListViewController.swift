// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import AVKit
import BraveCore
import BraveShared
import Combine
import CoreData
import Data
import Foundation
import Growth
import MediaPlayer
import Playlist
import Preferences
// Third-Party
import SDWebImage
import Shared
import SwiftUI
import os.log

// MARK: - PlaylistListViewController

class PlaylistListViewController: UIViewController {
  // MARK: Loading State
  enum LoadingState {
    case partial
    case loading
    case fullyLoaded
  }

  // MARK: Constants

  struct Constants {
    static let playListMenuHeaderRedactedIdentifier = "playListMenuHeaderRedactedIdentifier"
    static let playListMenuHeaderIdentifier = "playlistMenuHeaderIdentifier"
    static let playlistCellRedactedIdentifier = "playlistCellRedactedIdentifier"
    static let playListCellIdentifier = "playlistCellIdentifier"
    static let tableRowHeight: CGFloat = 80
    static let tableHeaderHeight: CGFloat = 11
    static let tableRedactedCellCount = 5
  }

  // MARK: Properties
  public var initialItem: PlaylistInfo?
  public var initialItemPlaybackOffset = 0.0

  weak var delegate: PlaylistViewControllerDelegate?
  private let braveCore: BraveCoreMain?
  private let playerView: VideoView
  let isPrivateBrowsing: Bool

  private var observers = Set<AnyCancellable>()
  private var folderObserver: AnyCancellable?
  private var sharedFolderLoadingTask: Task<Void, Error>?
  private(set) var autoPlayEnabled = Preferences.Playlist.firstLoadAutoPlay.value
  var playerController: AVPlayerViewController?
  var loadingState: LoadingState = .fullyLoaded {
    didSet {
      tableView.reloadData()
    }
  }

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

  init(braveCore: BraveCoreMain?, playerView: VideoView, isPrivateBrowsing: Bool) {
    self.braveCore = braveCore
    self.playerView = playerView
    self.isPrivateBrowsing = isPrivateBrowsing
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    PlaylistManager.shared.onFolderRemovedOrUpdated
      .sink { [weak self] in
        self?.tableView.reloadData()
      }.store(in: &observers)

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
        self?.controllerDidChange(
          $0.object,
          at: $0.indexPath,
          for: $0.type,
          newIndexPath: $0.newIndexPath
        )
      }.store(in: &observers)

    PlaylistManager.shared.downloadProgressUpdated
      .sink { [weak self] in
        self?.onDownloadProgressUpdate(
          id: $0.id,
          percentComplete: $0.percentComplete
        )
      }.store(in: &observers)

    PlaylistManager.shared.downloadStateChanged
      .sink { [weak self] in
        self?.onDownloadStateChanged(
          id: $0.id,
          state: $0.state,
          displayName: $0.displayName,
          error: $0.error
        )
      }.store(in: &observers)

    PlaylistCoordinator.shared.onCarplayUIChangedToRoot.eraseToAnyPublisher()
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        guard let self = self else { return }
        self.navigationController?.popToRootViewController(animated: true)
      }.store(in: &observers)

    // Theme
    title = Strings.PlayList.playListTitle
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
      $0.register(
        PlaylistRedactedHeader.self,
        forHeaderFooterViewReuseIdentifier: Constants.playListMenuHeaderRedactedIdentifier
      )
      $0.register(
        PlaylistMenuHeader.self,
        forHeaderFooterViewReuseIdentifier: Constants.playListMenuHeaderIdentifier
      )
      $0.register(
        PlaylistCellRedacted.self,
        forCellReuseIdentifier: Constants.playlistCellRedactedIdentifier
      )
      $0.register(PlaylistCell.self, forCellReuseIdentifier: Constants.playListCellIdentifier)
      $0.dataSource = self
      $0.delegate = self
      $0.dragDelegate = self
      $0.dropDelegate = self
      $0.dragInteractionEnabled = true
      $0.allowsMultipleSelectionDuringEditing = true
    }

    updateToolbar(editing: false)
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    title =
      PlaylistManager.shared.numberOfAssets == 0 ? PlaylistManager.shared.currentFolder?.title : nil
    navigationController?.setToolbarHidden(true, animated: true)

    // Update
    DispatchQueue.main.async {
      self.fetchResults()
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    highlightActiveItem()

    folderObserver = PlaylistManager.shared.onCurrentFolderDidChange
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        guard let self = self else { return }
        self.title =
          PlaylistManager.shared.numberOfAssets == 0
          ? PlaylistManager.shared.currentFolder?.title : nil
        self.tableView.reloadData()
      }
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)

    // Store the last played item's time-offset
    if let playTime = delegate?.currentPlaylistItem?.currentTime(),
      let item = PlaylistCoordinator.shared.currentPlaylistItem
    {
      PlaylistManager.shared.updateLastPlayed(item: item, playTime: playTime.seconds)
    }

    onCancelEditingItems()
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)

    folderObserver = nil
    if isMovingFromParent || isBeingDismissed {
      if !PlaylistCoordinator.shared.isCarPlayAvailable {
        delegate?.stopPlaying()
      }

      stopLoadingSharedPlaylist()

      if !PlaylistCoordinator.shared.isCarPlayAvailable {
        PlaylistCoordinator.shared.onCarplayUIChangedToRoot.send()
      }
    }
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    updateTableBackgroundView()
  }

  deinit {
    folderObserver = nil
    stopLoadingSharedPlaylist()
  }

  // MARK: Internal

  private func fetchResults() {
    updateTableBackgroundView()
    playerView.setControlsEnabled(false)

    if let initialItem = initialItem,
      let item = PlaylistItem.getItem(uuid: initialItem.tagId)
    {
      PlaylistManager.shared.currentFolder = item.playlistFolder
    }

    let initialItem = self.initialItem
    let initialItemOffset = self.initialItemPlaybackOffset
    self.initialItem = nil
    self.initialItemPlaybackOffset = 0.0

    PlaylistManager.shared.reloadData()
    tableView.reloadData()

    // After reloading all data, update the background
    guard PlaylistManager.shared.numberOfAssets > 0 else {
      updateTableBackgroundView()
      return
    }

    // Otherwise prepare to play the first item
    updateTableBackgroundView()
    playerView.setControlsEnabled(true)

    // Shared folders are loading
    if PlaylistManager.shared.currentFolder?.sharedFolderId != nil,
      PlaylistManager.shared.currentFolder?.isPersistent == false
    {
      autoPlayEnabled = false
      return
    }

    // If car play is active OR media is already playing, do nothing
    // We do nothing when CarPlay is active because the user shouldn't be using the phone anyway
    // But also because if the driver is controlling the audio, there will be a conflict
    // if both the driver is selecting an item, and auto-play happens.
    if PlaylistCoordinator.shared.isCarPlayAvailable
      || (delegate?.currentPlaylistAsset != nil || delegate?.isPlaying ?? false)
    {
      return
    }

    // Setup initial playback item and time-offset
    var lastPlayedItemTime: Double = 0.0

    let lastPlayedItemUrl = initialItem?.pageSrc ?? Preferences.Playlist.lastPlayedItemUrl.value
    let lastPlayedItem = PlaylistManager.shared.allItems.first(where: {
      $0.pageSrc == lastPlayedItemUrl
    })

    // If the user is current viewing the same video as the last played item
    // then we choose whichever time offset is more recent
    if let pageSrc = initialItem?.pageSrc, let lastPlayedItem = lastPlayedItem,
      pageSrc == lastPlayedItem.pageSrc
    {

      // User is current on the same page as the last played item]
      lastPlayedItemTime = max(initialItemOffset, lastPlayedItem.lastPlayedOffset)
    } else {
      // Otherwise we choose the last played item offset from the page and fallback to the preference if none exists
      lastPlayedItemTime =
        initialItem != nil ? initialItemOffset : lastPlayedItem?.lastPlayedOffset ?? 0.0
    }

    // Auto-play is based on loading state and preference only
    autoPlayEnabled =
      loadingState != .loading ? Preferences.Playlist.firstLoadAutoPlay.value : false

    // Should load determines if the player should load the item, but does not "auto-play" it!
    let shouldLoad = initialItem != nil ? true : autoPlayEnabled

    if !shouldLoad {
      delegate?.showStaticImage(image: nil)
      delegate?.stopPlaying()
      return
    }

    // If there is no last played item, then just select the first item in the playlist
    // which will play it if auto-play is enabled.
    guard let lastPlayedItemId = lastPlayedItem?.tagId,
      let index = PlaylistManager.shared.index(of: lastPlayedItemId)
    else {
      tableView.delegate?.tableView?(tableView, didSelectRowAt: IndexPath(row: 0, section: 0))
      return
    }

    // If the current item is already playing, do nothing.
    if let currentItemId = PlaylistCoordinator.shared.currentPlaylistItem?.tagId,
      PlaylistManager.shared.index(of: currentItemId) != nil
    {
      return
    }

    // Prepare the UI before playing the item
    delegate?.showStaticImage(image: nil)
    let indexPath = IndexPath(row: index, section: 0)
    prepareToPlayItem(at: indexPath) { [weak self] item in
      guard let self = self,
        let delegate = self.delegate,
        let item = item
      else {
        self?.commitPlayerItemTransaction(
          at: indexPath,
          isExpired: false
        )
        return
      }

      PlaylistManager.shared.playbackTask = Task { @MainActor in
        do {
          PlaylistCoordinator.shared.currentlyPlayingItemIndex = indexPath.row
          PlaylistCoordinator.shared.currentPlaylistItem = item

          let item = try await delegate.playItem(item: item)

          self.commitPlayerItemTransaction(
            at: indexPath,
            isExpired: false
          )

          // Update the player position/time-offset of the last played item
          self.seekLastPlayedItem(
            at: indexPath,
            lastPlayedItemId: lastPlayedItemId,
            lastPlayedTime: lastPlayedItemTime
          )

          // Even if the item was NOT previously the last played item,
          // it is now as it has begun to play
          delegate.updateLastPlayedItem(item: item)
        } catch {
          PlaylistCoordinator.shared.currentPlaylistItem = nil
          PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1

          self.commitPlayerItemTransaction(
            at: indexPath,
            isExpired: false
          )

          delegate.displayLoadingResourceError()
          Logger.module.error("Playlist Playback Error: \(error)")
        }
      }
    }
  }

  func seekLastPlayedItem(at indexPath: IndexPath, lastPlayedItemId: String, lastPlayedTime: Double)
  {
    // The item can be deleted at any time,
    // so we need to guard against it and make sure the index path matches up correctly
    // If it does, we check the last played time
    // and seek to that position in the media item
    let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
    guard let item = item else { return }

    if item.tagId == lastPlayedItemId && lastPlayedTime > 0.0
      && lastPlayedTime < delegate?.currentPlaylistAsset?.duration.seconds ?? 0.0
      && Preferences.Playlist.playbackLeftOff.value
    {
      self.playerView.seek(to: lastPlayedTime)
    }
  }

  // MARK: Actions

  func updateToolbar(editing: Bool) {
    (tableView.headerView(forSection: 0) as? PlaylistMenuHeader)?.setMenuEnabled(enabled: !editing)

    if editing {
      if PlaylistFolder.getOtherFoldersCount() == 0 {
        toolbarItems = [
          UIBarButtonItem(
            title: Strings.cancelButtonTitle,
            style: .plain,
            target: self,
            action: #selector(onCancelEditingItems)
          ),
          UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
          UIBarButtonItem(
            title: Strings.delete,
            style: .plain,
            target: self,
            action: #selector(onDeleteEditingItems)
          ),
        ]
      } else {
        toolbarItems = [
          UIBarButtonItem(
            title: Strings.cancelButtonTitle,
            style: .plain,
            target: self,
            action: #selector(onCancelEditingItems)
          ),
          UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
          UIBarButtonItem(
            title: Strings.PlaylistFolders.playlistFolderMoveFolderButtonTitle,
            style: .plain,
            target: self,
            action: #selector(onMoveEditingItems)
          ).then {
            $0.isEnabled = !(tableView.indexPathsForSelectedRows?.isEmpty ?? true)
          },
          UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
          UIBarButtonItem(
            title: Strings.delete,
            style: .plain,
            target: self,
            action: #selector(onDeleteEditingItems)
          ),
        ]
      }
    } else {
      toolbarItems = nil
    }

    navigationController?.setToolbarHidden(!editing, animated: true)
  }

  func moveItems(indexPaths: [IndexPath]) {
    onCancelEditingItems()

    let selectedItems = indexPaths.compactMap({
      PlaylistManager.shared.fetchedObjects[safe: $0.row]
    })

    if selectedItems.contains(where: {
      $0.uuid == PlaylistCoordinator.shared.currentPlaylistItem?.tagId
    }) {
      delegate?.stopPlaying()
    } else {
      delegate?.pausePlaying()
    }

    var moveController = PlaylistMoveFolderView(selectedItems: selectedItems)
    moveController.onCancelButtonPressed = { [weak self] in
      self?.presentedViewController?.dismiss(animated: true, completion: nil)
    }

    moveController.onDoneButtonPressed = { [weak self] items, folder in
      guard let self = self else { return }
      self.presentedViewController?.dismiss(animated: true, completion: nil)

      // We moved an item that was playing
      if items.firstIndex(where: {
        PlaylistInfo(item: $0).tagId == PlaylistCoordinator.shared.currentPlaylistItem?.tagId
      }) != nil {
        self.delegate?.stopPlaying()
      }

      // We moved all items in this folder
      if items.count == selectedItems.count {
        self.navigationController?.popViewController(animated: true)
      }

      PlaylistItem.moveItems(items: items.map({ $0.objectID }), to: folder?.uuid)
    }

    let hostingController = UIHostingController(
      rootView: moveController.environment(\.managedObjectContext, DataController.swiftUIContext)
    ).then {
      $0.modalPresentationStyle = .formSheet
    }

    present(hostingController, animated: true, completion: nil)
  }

  @objc
  private func onExit(_ button: UIBarButtonItem) {
    dismiss(animated: true) {
      // Handle App Rating
      // User finished viewing the playlist list view.
      AppReviewManager.shared.handleAppReview(for: .revised, using: self)
    }
  }

  @objc
  func onEditItems() {
    if tableView.isEditing {
      // If already editing, such as when swiping on a cell,
      // dismiss the trailing swipe and show the selections instead.
      tableView.setEditing(false, animated: false)
      tableView.setEditing(true, animated: false)
    } else {
      tableView.setEditing(true, animated: true)
    }

    updateToolbar(editing: true)
  }

  @objc
  func onCancelEditingItems() {
    tableView.setEditing(false, animated: true)
    updateToolbar(editing: false)
  }

  @objc
  private func onMoveEditingItems() {
    moveItems(indexPaths: tableView.indexPathsForSelectedRows ?? [])
  }

  @objc
  private func onDeleteEditingItems() {
    let selection = tableView.indexPathsForSelectedRows ?? []
    onCancelEditingItems()

    let rows = selection.map({
      (index: $0.row, item: PlaylistManager.shared.itemAtIndex($0.row))
    })

    for row in rows {
      if let item = row.item {
        delegate?.deleteItem(itemId: item.tagId, at: row.index)
      }
    }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    .lightContent
  }

  public func updateLayoutForMode(_ mode: UIUserInterfaceIdiom) {
    navigationItem.rightBarButtonItem = nil

    if mode == .phone {
      navigationItem.rightBarButtonItem = UIBarButtonItem(
        barButtonSystemItem: .done,
        target: self,
        action: #selector(onExit(_:))
      )

      playerView.setSidePanelHidden(true)

      // If the player view is in fullscreen, we should NOT change the tableView layout on rotation.
      view.addSubview(tableView)
      view.addSubview(playerView)
      playerView.addSubview(activityIndicator)

      if !playerView.isFullscreen {
        if view.window?.windowScene?.interfaceOrientation.isLandscape
          ?? UIDevice.current.orientation.isLandscape && UIDevice.isPhone
        {
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
          let videoPlayerHeight =
            (1.0 / 3.0)
            * (UIScreen.main.bounds.width > UIScreen.main.bounds.height
              ? UIScreen.main.bounds.width : UIScreen.main.bounds.height)

          playerView.snp.remakeConstraints {
            $0.top.equalTo(view.safeArea.top)
            $0.leading.trailing.equalToSuperview()
            $0.height.equalTo(videoPlayerHeight)
          }

          activityIndicator.snp.remakeConstraints {
            $0.center.equalToSuperview()
          }

          tableView.snp.remakeConstraints {
            $0.leading.trailing.equalToSuperview()
            $0.top.equalTo(playerView.snp.bottom)
            $0.bottom.equalTo(view.safeArea.bottom)
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

      activityIndicator.snp.remakeConstraints {
        $0.center.equalToSuperview()
      }

      tableView.snp.remakeConstraints {
        $0.edges.equalToSuperview()
      }
    }
  }

  override func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)

    if UIDevice.isPhone && splitViewController?.isCollapsed == true {
      updateLayoutForMode(.phone)

      if !playerView.isFullscreen {
        navigationController?.setNavigationBarHidden(
          UIDevice.current.orientation.isLandscape,
          animated: true
        )
      }
    }
  }
}

extension PlaylistListViewController {
  func updateTableBackgroundView() {
    if PlaylistManager.shared.numberOfAssets > 0 || loadingState != .fullyLoaded {
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

        let offset = abs(tableView.contentOffset.y)
        $0.frame.center.x = tableView.bounds.center.x
        $0.frame.origin.y = offset + ((tableView.bounds.height - offset) / 2.0)
      }

      tableView.backgroundView = UIView().then {
        $0.addSubview(messageLabel)
      }
      tableView.separatorStyle = .none
      navigationController?.setToolbarHidden(true, animated: true)
    }
  }

  func prepareToPlayItem(at indexPath: IndexPath, _ completion: ((PlaylistInfo?) -> Void)?) {
    // Update the UI in preparation to play an item
    // Show the activity indicator, update the cell and player view, etc.
    guard indexPath.row < PlaylistManager.shared.numberOfAssets,
      let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
    else {
      completion?(nil)
      return
    }

    playerView.pause()
    playerView.bringSubviewToFront(activityIndicator)
    activityIndicator.startAnimating()
    activityIndicator.isHidden = false

    let selectedCell = tableView.cellForRow(at: indexPath) as? PlaylistCell
    playerView.setVideoInfo(
      videoDomain: item.pageSrc,
      videoTitle: item.pageTitle,
      isPrivateBrowsing: isPrivateBrowsing
    )
    NowPlayingInfo.setNowPlayingMediaArtwork(image: selectedCell?.iconView.image)
    completion?(item)
  }

  func commitPlayerItemTransaction(at indexPath: IndexPath, isExpired: Bool) {
    if isExpired {
      let selectedCell = tableView.cellForRow(at: indexPath) as? PlaylistCell
      selectedCell?.detailLabel.text = Strings.PlayList.expiredLabelTitle
    }

    playerView.setControlsEnabled(!isExpired)
    activityIndicator.stopAnimating()
  }

  func highlightActiveItem() {
    let activeItemIndex = PlaylistCoordinator.shared.currentlyPlayingItemIndex

    tableView.selectRow(
      at: IndexPath(row: activeItemIndex, section: 0),
      animated: false,
      scrollPosition: .none
    )
  }

  func loadSharedPlaylist(folderSharingUrl: String) {
    stopLoadingSharedPlaylist()

    sharedFolderLoadingTask = Task { @MainActor in
      // Shared Folder already exists
      if let existingFolder = PlaylistFolder.getSharedFolder(sharedFolderUrl: folderSharingUrl) {
        PlaylistManager.shared.currentFolder = existingFolder
        self.loadingState = .fullyLoaded
        return
      }

      // Shared Folder doesn't exist
      do {
        self.playerView.setStaticImage(image: UIImage())
        let model = try await PlaylistSharedFolderNetwork.fetchPlaylist(folderUrl: folderSharingUrl)
        let folder = await PlaylistSharedFolderNetwork.createInMemoryStorage(for: model)
        PlaylistManager.shared.currentFolder = folder
        self.loadingState = .partial

        if let folderImageUrl = model.folderImage {
          let authManager = BasicAuthCredentialsManager()
          let session = URLSession(
            configuration: .ephemeral,
            delegate: authManager,
            delegateQueue: .main
          )

          try await withTaskCancellationHandler {
            defer { session.finishTasksAndInvalidate() }

            let (data, response) = try await NetworkManager(session: session).dataRequest(
              with: folderImageUrl
            )
            if let response = response as? HTTPURLResponse, response.statusCode != 200 {
              return
            }

            if let image = UIImage(data: data, scale: UIScreen.main.scale) {
              self.playerView.setStaticImage(image: image)
            }
          } onCancel: {
            session.invalidateAndCancel()
          }
        }

        let items = try await PlaylistSharedFolderNetwork.fetchMediaItemInfo(
          item: model,
          viewForInvisibleWebView: self.playerView.window ?? self.playerView.superview
            ?? self.playerView,
          webLoaderFactory: LivePlaylistWebLoaderFactory(braveCore: self.braveCore)
        )
        try Task.checkCancellation()

        try folder.playlistItems?.forEach({ playlistItem in
          try Task.checkCancellation()

          if let item = items.first(where: { $0.tagId == playlistItem.uuid }) {
            playlistItem.name = item.name
            playlistItem.pageTitle = item.pageTitle
            playlistItem.pageSrc = item.pageSrc
            playlistItem.duration = item.duration
            playlistItem.mimeType = item.mimeType
            playlistItem.mediaSrc = item.src
            playlistItem.uuid = item.tagId
            playlistItem.order = item.order
          }
        })

        self.loadingState = .fullyLoaded
      } catch {
        if let error = error as? PlaylistSharedFolderNetwork.Status {
          Logger.module.error("\(error.localizedDescription)")
        } else {
          Logger.module.error(
            "Failed Fetching Playlist Shared Folder: \(error.localizedDescription)"
          )
        }

        displayLoadingResourceError()
      }
    }
  }

  func stopLoadingSharedPlaylist() {
    sharedFolderLoadingTask?.cancel()
    sharedFolderLoadingTask = nil
  }
}

// MARK: - Error Handling

extension PlaylistListViewController {
  func displayExpiredResourceError(item: PlaylistInfo?) {
    if let item = item {
      let alert = UIAlertController(
        title: Strings.PlayList.expiredAlertTitle,
        message: Strings.PlayList.expiredAlertDescription,
        preferredStyle: .alert
      )
      alert.addAction(
        UIAlertAction(
          title: Strings.PlayList.reopenButtonTitle,
          style: .default,
          handler: { _ in

            if let url = URL(string: item.pageSrc) {
              self.dismiss(animated: true, completion: nil)
              let isPrivateBrowsing = self.isPrivateBrowsing
              self.delegate?.openURLInNewTab(
                url,
                isPrivate: isPrivateBrowsing,
                isPrivileged: false
              )
            }
          }
        )
      )
      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
      self.present(alert, animated: true, completion: nil)
    } else {
      let alert = UIAlertController(
        title: Strings.PlayList.expiredAlertTitle,
        message: Strings.PlayList.expiredAlertDescription,
        preferredStyle: .alert
      )
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }

  func displayLoadingResourceError() {
    let alert = UIAlertController(
      title: Strings.PlayList.sorryAlertTitle,
      message: Strings.PlayList.loadResourcesErrorAlertDescription,
      preferredStyle: .alert
    )
    alert.addAction(
      UIAlertAction(title: Strings.PlayList.okayButtonTitle, style: .default, handler: nil)
    )

    self.present(alert, animated: true, completion: nil)
  }
}

extension PlaylistListViewController: UIGestureRecognizerDelegate {
  func gestureRecognizer(
    _ gestureRecognizer: UIGestureRecognizer,
    shouldReceive touch: UITouch
  ) -> Bool {
    return !tableView.isEditing
  }
}

// MARK: VideoViewDelegate

extension PlaylistListViewController {

  func onFullscreen() {
    navigationController?.setNavigationBarHidden(true, animated: true)
    navigationController?.setToolbarHidden(true, animated: true)

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
    if let browser = PlaylistCoordinator.shared.browserController,
      let currentItem = PlaylistCoordinator.shared.currentPlaylistItem,
      let pageURL = URL(string: currentItem.pageSrc)
    {

      self.dismiss(animated: true) {
        let isPrivateBrowsing = self.isPrivateBrowsing
        browser.tabManager.addTabAndSelect(
          URLRequest(url: pageURL),
          isPrivate: isPrivateBrowsing
        )
      }
    }
  }
}

// MARK: - PlaylistManagerDelegate

extension PlaylistListViewController {
  func updateCellDownloadStatus(
    indexPath: IndexPath,
    cell: PlaylistCell?,
    state: PlaylistDownloadManager.DownloadState,
    percentComplete: Double?
  ) {

    guard let cell = cell ?? tableView.cellForRow(at: indexPath) as? PlaylistCell else {
      return
    }

    guard let item = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
      return
    }

    cell.itemId = item.id

    switch state {
    case .inProgress:
      if let percentComplete = percentComplete {
        getAssetDurationFormatted(item: item) { [weak cell] in
          guard cell?.itemId == item.id else { return }
          cell?.detailLabel.text =
            "\($0) - \(Int(percentComplete))% \(Strings.PlayList.savedForOfflineLabelTitle)"
        }
      } else {
        getAssetDurationFormatted(item: item) { [weak cell] in
          cell?.detailLabel.text = "\($0) - \(Strings.PlayList.savingForOfflineLabelTitle)"
        }
      }

    case .downloaded:
      if let itemSize = PlaylistManager.shared.sizeOfDownloadedItemSynchronous(for: item.tagId) {
        getAssetDurationFormatted(item: item) { [weak cell] in
          guard cell?.itemId == item.id else { return }
          cell?.detailLabel.text = "\($0) - \(itemSize)"
        }
      } else {
        getAssetDurationFormatted(item: item) { [weak cell] in
          guard cell?.itemId == item.id else { return }
          cell?.detailLabel.text = "\($0) - \(Strings.PlayList.savedForOfflineLabelTitle)"
        }
      }

    case .invalid:
      getAssetDurationFormatted(item: item) { [weak cell] in
        guard cell?.itemId == item.id else { return }
        cell?.detailLabel.text = $0
      }
    }
  }

  func onDownloadProgressUpdate(id: String, percentComplete: Double) {
    guard let index = PlaylistManager.shared.index(of: id) else {
      return
    }

    // Cell is not visible, do not update percentages
    let indexPath = IndexPath(row: index, section: 0)
    if tableView.indexPathsForVisibleRows?.contains(indexPath) == false {
      return
    }

    updateCellDownloadStatus(
      indexPath: indexPath,
      cell: nil,
      state: .inProgress,
      percentComplete: percentComplete
    )
  }

  func onDownloadStateChanged(
    id: String,
    state: PlaylistDownloadManager.DownloadState,
    displayName: String?,
    error: Error?
  ) {
    guard let index = PlaylistManager.shared.index(of: id) else {
      return
    }

    // Cell is not visible, do not update status
    let indexPath = IndexPath(row: index, section: 0)
    if tableView.indexPathsForVisibleRows?.contains(indexPath) == false {
      return
    }

    guard let error = error else {
      updateCellDownloadStatus(
        indexPath: indexPath,
        cell: nil,
        state: state,
        percentComplete: nil
      )
      return
    }

    // Some sort of error happened while downloading the playlist item
    Logger.module.error("Error downloading playlist item: \(error.localizedDescription)")

    guard let cell = tableView.cellForRow(at: indexPath) as? PlaylistCell else {
      return
    }

    // Show only the item duration on the cell
    updateCellDownloadStatus(
      indexPath: indexPath,
      cell: cell,
      state: .invalid,
      percentComplete: nil
    )

    let alert = UIAlertController(
      title: Strings.PlayList.playlistSaveForOfflineErrorTitle,
      message: Strings.PlayList.playlistSaveForOfflineErrorMessage,
      preferredStyle: .alert
    ).then {

      $0.addAction(
        UIAlertAction(
          title: Strings.PlayList.okayButtonTitle,
          style: .default,
          handler: nil
        )
      )
    }
    self.present(alert, animated: true, completion: nil)
  }

  func controllerDidChange(
    _ anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {
    if tableView.hasActiveDrag || tableView.hasActiveDrop { return }
    tableView.reloadData()
  }

  func controllerDidChangeContent() {
    if tableView.hasActiveDrag || tableView.hasActiveDrop { return }
    tableView.reloadData()
  }

  func controllerWillChangeContent() {
    if tableView.hasActiveDrag || tableView.hasActiveDrop { return }
    tableView.reloadData()
  }
}
