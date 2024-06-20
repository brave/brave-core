// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import CarPlay
import Combine
import CoreData
import Data
import Foundation
import Playlist
import Preferences
import Strings

/// Handles display and input of audio on CarPlay
///
/// This class populates the CarPlay interface and binds the UI to the player model.
public class CarPlayController {
  public let player: PlayerModel
  public let interface: CPInterfaceController

  private let playLaterFolder = PlaylistFolder.getFolder(uuid: PlaylistFolder.savedFolderUUID)
  private let foldersFRC = PlaylistFolder.frc(savedFolder: false, sharedFolders: false)
  private let fetchResultsDelegate: FetchResultsDelegate

  public init(
    player: PlayerModel,
    interface: CPInterfaceController
  ) {
    self.player = player
    self.interface = interface
    self.fetchResultsDelegate = FetchResultsDelegate()

    foldersFRC.delegate = fetchResultsDelegate
    try? foldersFRC.performFetch()

    Task { @MainActor in
      await player.prepareItemQueue()

      let folderList = folderListTemplate
      self.currentFolderListTemplate = folderList
      interface.setRootTemplate(
        CPTabBarTemplate(templates: [folderList, settingsTemplate]),
        animated: true,
        completion: nil
      )
    }

    fetchResultsDelegate.contentDidChange = { [weak self] in
      guard let self else { return }
      Task { @MainActor in
        self.updateFoldersList()
        self.updateItemList()
      }
    }
    setUpPlayerBindings()
  }

  private var currentFolderListTemplate: CPListTemplate?
  private var currentItemListTemplate: CPListTemplate?
  private var selectedFolderID: PlaylistFolder.ID?
  private var isErrorPresented: Bool = false

  @MainActor private func updateFoldersList() {
    let folderListTemplate = self.folderListTemplate
    currentFolderListTemplate?.updateSections(folderListTemplate.sections)
  }

  @MainActor private func updateItemList() {
    guard let selectedFolderID else { return }
    let itemListTemplate = self.itemListTemplate(for: selectedFolderID)
    currentItemListTemplate?.updateSections(itemListTemplate.sections)
  }

  @MainActor private func handlePlayerError() {
    if isErrorPresented, player.error == nil, interface.presentedTemplate is CPAlertTemplate {
      // Handle the case where a user dismisses an error in the app while CarPlay is open
      interface.dismissTemplate(animated: true) { [weak self] _, _ in
        self?.isErrorPresented = false
      }
      return
    }

    guard let error = player.error, let title = error.failureReason ?? error.errorDescription,
      !isErrorPresented
    else {
      return
    }

    let alert = CPAlertTemplate(
      titleVariants: [title],
      actions: [
        CPAlertAction(
          title: Strings.PlayList.okayButtonTitle,
          style: .default,
          handler: { [weak self] _ in
            self?.interface.dismissTemplate(
              animated: true,
              completion: { success, error in
                guard let self else { return }
                self.player.error?.handler?()
                self.player.error = nil
                self.isErrorPresented = false
                if self.interface.topTemplate == CPNowPlayingTemplate.shared {
                  self.interface.popTemplate(animated: true, completion: nil)
                }
              }
            )
          }
        )
      ]
    )

    // Preemptively set this so repeat object changes dont present multiple times
    isErrorPresented = true
    interface.presentTemplate(
      alert,
      animated: true,
      completion: { [weak self] success, _ in
        if !success {
          self?.isErrorPresented = false
        }
      }
    )
  }

  private var cancellables: Set<AnyCancellable> = []
  private func setUpPlayerBindings() {
    player.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self] in
        MainActor.assumeIsolated {
          self?.updateItemList()
          self?.handlePlayerError()
        }
      }
      .store(in: &cancellables)

    PlaylistManager.shared.downloadStateChanged
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in
        MainActor.assumeIsolated {
          self?.updateItemList()
        }
      }
      .store(in: &cancellables)
  }

  // MARK: -

  @MainActor private func handleItemTap(_ item: PlaylistItem) {
    if player.selectedItemID != item.id {
      player.selectedItemID = item.id
      // FIXME: Move this into PlayerModel
      player.makeItemQueue(selectedItemID: item.id)
      // Technically we don't need to set this folder at all, but we could
      // player.selectedFolderID = item.playlistFolder?.id
      Task {
        await player.prepareToPlaySelectedItem(initialOffset: nil, playImmediately: true)
      }
    } else {
      // Tapped the currently active item, resume playback or reset based on user pref
      if Preferences.Playlist.enableCarPlayRestartPlayback.value {
        Task {
          await player.seek(to: 0)
          player.play()
        }
      } else {
        player.play()
      }
    }
    if interface.topTemplate != CPNowPlayingTemplate.shared {
      interface.pushTemplate(CPNowPlayingTemplate.shared, animated: true, completion: nil)
    }
  }

  // MARK: - Template Generation

  @MainActor private func itemListTemplate(for folderID: PlaylistFolder.ID) -> CPListTemplate {
    guard let folder = PlaylistFolder.getFolder(uuid: folderID) else {
      return CPListTemplate(title: nil, sections: [])
    }
    let items: [CPListItem] = PlaylistItem.getItems(parentFolder: folder)
      .compactMap { item in
        guard let itemUUID = item.uuid else { return nil }
        let listItem = item.listItem
        listItem.accessoryType =
          PlaylistManager.shared.state(for: itemUUID) != .downloaded ? .cloud : .none
        listItem.isPlaying = player.isPlaying && player.selectedItemID == item.id
        listItem.playingIndicatorLocation = .trailing
        listItem.userInfo = ["id": item.id, "uuid": itemUUID]
        listItem.handler = { [unowned self] _, completion in
          handleItemTap(item)
          completion()
        }
        return listItem
      }
    return CPListTemplate(
      title: folder.title ?? "",
      sections: [.init(items: items)]
    )
  }

  @MainActor private var folderListTemplate: CPListTemplate {
    var folders = (foldersFRC.fetchedObjects ?? [])
    if let playLaterFolder {
      folders.insert(playLaterFolder, at: 0)
    }
    var items: [CPListItem] = []
    for folder in folders {
      let listItem = folder.listItem
      listItem.handler = { [weak self, id = folder.id] _, completion in
        guard let self else { return }
        let template = itemListTemplate(for: id)
        currentItemListTemplate = template
        selectedFolderID = id
        interface.pushTemplate(template, animated: true, completion: nil)
        completion()
      }
      items.append(listItem)
    }

    let template = CPListTemplate(
      title: Strings.PlayList.playlistCarplayTitle,
      sections: [
        .init(items: items)
      ]
    )
    template.tabImage = UIImage(braveSystemNamed: "leo.product.playlist")
    template.emptyViewTitleVariants = [Strings.PlayList.noItemLabelTitle]
    template.emptyViewSubtitleVariants = [Strings.PlayList.noItemLabelDetailLabel]
    return template
  }

  private var settingsTemplate: CPListTemplate {
    var restartPlaybackAccessoryImage: UIImage? {
      Preferences.Playlist.enableCarPlayRestartPlayback.value
        ? UIImage(braveSystemNamed: "leo.checkbox.checked")
        : UIImage(braveSystemNamed: "leo.checkbox.unchecked")
    }
    let restartPlaybackItem = CPListItem(
      text: Strings.PlayList.playlistCarplayRestartPlaybackOptionTitle,
      detailText: Strings.PlayList.playlistCarplayRestartPlaybackOptionDetailsTitle,
      image: nil,
      accessoryImage: restartPlaybackAccessoryImage,
      accessoryType: .none
    )
    restartPlaybackItem.handler = { _, completion in
      Preferences.Playlist.enableCarPlayRestartPlayback.value.toggle()
      restartPlaybackItem.setAccessoryImage(restartPlaybackAccessoryImage)
      completion()
    }
    let template = CPListTemplate(
      title: Strings.PlayList.playlistCarplaySettingsSectionTitle,
      sections: [
        .init(
          items: [restartPlaybackItem],
          header: Strings.PlayList.playlistCarplayOptionsScreenTitle,
          sectionIndexTitle: Strings.PlayList.playlistCarplayOptionsScreenTitle
        )
      ]
    )
    template.tabImage = UIImage(braveSystemNamed: "leo.settings")
    return template
  }
}

extension PlaylistFolder {
  fileprivate var listItem: CPListItem {
    let itemCount = playlistItems?.count ?? 0
    return .init(
      text: title ?? Strings.PlaylistFolders.playlistUntitledFolderTitle,
      detailText:
        "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
      image: nil,
      accessoryImage: nil,
      accessoryType: .disclosureIndicator
    )
  }
}

extension PlaylistItem {
  fileprivate var listItem: CPListItem {
    .init(text: name, detailText: pageSrc)
  }
}

private class FetchResultsDelegate: NSObject, NSFetchedResultsControllerDelegate {
  var contentDidChange: (() -> Void)?

  func controllerDidChangeContent(
    _ controller: NSFetchedResultsController<any NSFetchRequestResult>
  ) {
    self.contentDidChange?()
  }
}
