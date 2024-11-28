// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Playlist
import Preferences
import Shared
import Static
import UIKit

// MARK: - PlayListSide

extension PlayListSide: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .left:
      return Strings.PlayList.playlistSidebarLocationOptionLeft
    case .right:
      return Strings.PlayList.playlistSidebarLocationOptionRight
    }
  }
}

// MARK: - PlayListDownloadType

extension PlayListDownloadType: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .on:
      return Strings.PlayList.playlistAutoSaveOptionOn
    case .off:
      return Strings.PlayList.playlistAutoSaveOptionOff
    case .wifi:
      return Strings.PlayList.playlistAutoSaveOptionOnlyWifi
    }
  }
}

// MARK: - PlaylistSettingsViewController

class PlaylistSettingsViewController: TableViewController {

  // MARK: Lifecycle

  init() {
    super.init(style: .insetGrouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.PlayList.playListTitle

    dataSource.sections = [
      Section(
        rows: [
          .boolRow(
            title: Strings.PlayList.urlBarButtonOptionTitle,
            option: Preferences.Playlist.enablePlaylistURLBarButton
          )
        ],
        footer: .title(Strings.PlayList.urlBarButtonOptionFooter)
      ),
      Section(
        rows: [
          .boolRow(
            title: Strings.PlayList.menuBadgeOptionTitle,
            option: Preferences.Playlist.enablePlaylistMenuBadge
          )
        ],
        footer: .title(Strings.PlayList.menuBadgeOptionFooterText)
      ),
      Section(
        rows: [
          .boolRow(
            title: Strings.PlayList.playlistLongPressSettingsOptionTitle,
            option: Preferences.Playlist.enableLongPressAddToPlaylist
          )
        ],
        footer: .title(Strings.PlayList.playlistLongPressSettingsOptionFooterText)
      ),
      Section(
        rows: [
          .boolRow(
            title: Strings.PlayList.playlistAutoPlaySettingsOptionTitle,
            option: Preferences.Playlist.firstLoadAutoPlay
          )
        ],
        footer: .title(Strings.PlayList.playlistAutoPlaySettingsOptionFooterText)
      ),
    ]

    var autoDownloadSection = Section(
      rows: [],
      footer: .title(Strings.PlayList.playlistAutoSaveSettingsFooterText)
    )
    var row = Row(
      text: Strings.PlayList.playlistAutoSaveSettingsTitle,
      detailText: PlayListDownloadType(rawValue: Preferences.Playlist.autoDownloadVideo.value)?
        .displayString,
      accessory: .disclosureIndicator,
      cellClass: MultilineSubtitleCell.self
    )

    row.selection = { [unowned self] in
      let optionsViewController = OptionSelectionViewController<PlayListDownloadType>(
        options: PlayListDownloadType.allCases,
        selectedOption: PlayListDownloadType(
          rawValue: Preferences.Playlist.autoDownloadVideo.value
        ),
        optionChanged: { [unowned self] _, option in
          Preferences.Playlist.autoDownloadVideo.value = option.rawValue

          self.dataSource.reloadCell(
            row: row,
            section: autoDownloadSection,
            displayText: option.displayString
          )
        }
      )
      optionsViewController.title = Strings.PlayList.playlistAutoSaveSettingsTitle
      optionsViewController.headerText = Strings.PlayList.playlistAutoSaveSettingsDescription

      self.navigationController?.pushViewController(optionsViewController, animated: true)
    }

    autoDownloadSection.rows.append(row)
    dataSource.sections.append(autoDownloadSection)

    dataSource.sections.append(
      Section(
        rows: [
          .boolRow(
            title: Strings.PlayList.playlistStartPlaybackSettingsOptionTitle,
            option: Preferences.Playlist.playbackLeftOff
          )
        ],
        footer: .title(Strings.PlayList.playlistStartPlaybackSettingsFooterText)
      )
    )

    if UIDevice.current.userInterfaceIdiom == .pad, !FeatureList.kNewPlaylistUI.enabled {
      var sideSelectionSection = Section(rows: [])
      var row = Row(
        text: Strings.PlayList.playlistSidebarLocationTitle,
        detailText: PlayListSide(rawValue: Preferences.Playlist.listViewSide.value)?.displayString,
        accessory: .disclosureIndicator,
        cellClass: MultilineSubtitleCell.self
      )

      row.selection = { [unowned self] in
        let optionsViewController = OptionSelectionViewController<PlayListSide>(
          options: PlayListSide.allCases,
          selectedOption: PlayListSide(rawValue: Preferences.Playlist.listViewSide.value),
          optionChanged: { [unowned self] _, option in
            Preferences.Playlist.listViewSide.value = option.rawValue

            self.dataSource.reloadCell(
              row: row,
              section: sideSelectionSection,
              displayText: option.displayString
            )
          }
        )
        optionsViewController.title = Strings.PlayList.playlistSidebarLocationTitle
        optionsViewController.footerText = Strings.PlayList.playlistSidebarLocationFooterText

        self.navigationController?.pushViewController(optionsViewController, animated: true)
      }

      sideSelectionSection.rows.append(row)
      dataSource.sections.append(sideSelectionSection)
    }

    dataSource.sections.append(
      Section(
        rows: [
          .boolRow(
            title: Strings.PlaylistFolderSharing.sharedFolderSyncAutomaticallyTitle,
            option: Preferences.Playlist.syncSharedFoldersAutomatically
          )
        ],
        footer: .title(Strings.PlaylistFolderSharing.sharedFolderSyncAutomaticallyDescription)
      )
    )

    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.PlayList.playlistResetAlertTitle,
            selection: { [unowned self] in
              let style: UIAlertController.Style =
                UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
              let alert = UIAlertController(
                title: Strings.PlayList.playlistResetAlertTitle,
                message: Strings.PlayList.playlistResetPlaylistOptionFooterText,
                preferredStyle: style
              )

              alert.addAction(
                UIAlertAction(
                  title: Strings.PlayList.playlistResetAlertTitle,
                  style: .default,
                  handler: { _ in
                    PlaylistCoordinator.shared.destroyPiP()
                    Task {
                      await PlaylistManager.shared.deleteAllItems(cacheOnly: false)
                    }
                  }
                )
              )
              alert.addAction(
                UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil)
              )
              self.present(alert, animated: true, completion: nil)
            },
            cellClass: ButtonCell.self
          )
        ],
        footer: .title(Strings.PlayList.playlistResetPlaylistOptionFooterText)
      )
    )
  }
}
