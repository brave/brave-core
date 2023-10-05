// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveShared
import BraveUI
import Shared
import Data
import MediaPlayer
import os.log
import Preferences
import Playlist

private extension PlaylistListViewController {
  func shareItem(_ item: PlaylistInfo, anchorView: UIView?) {
    guard let url = URL(string: item.pageSrc) else {
      return
    }

    let activityViewController = UIActivityViewController(
      activityItems: [url],
      applicationActivities: nil)

    activityViewController.excludedActivityTypes = [.openInIBooks, .saveToCameraRoll, .assignToContact]
    if UIDevice.current.userInterfaceIdiom == .pad {
      activityViewController.popoverPresentationController?.sourceView = anchorView ?? self.view
    }
    self.present(activityViewController, animated: true, completion: nil)
  }

  func cacheItem(_ item: PlaylistInfo, indexPath: IndexPath, cacheState: PlaylistDownloadManager.DownloadState) {
    switch cacheState {
    case .inProgress:
      PlaylistManager.shared.cancelDownload(itemId: item.tagId)
      tableView.reloadRows(at: [indexPath], with: .automatic)
    case .invalid:
      if PlaylistManager.shared.isDiskSpaceEncumbered() {
        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
        let alert = UIAlertController(
          title: Strings.PlayList.playlistDiskSpaceWarningTitle, message: Strings.PlayList.playlistDiskSpaceWarningMessage, preferredStyle: style)

        alert.addAction(
          UIAlertAction(
            title: Strings.OKString, style: .default,
            handler: { [unowned self] _ in
              PlaylistManager.shared.download(item: item)
              self.tableView.reloadRows(at: [indexPath], with: .automatic)
            }))

        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        self.present(alert, animated: true, completion: nil)
      } else {
        PlaylistManager.shared.download(item: item)
        tableView.reloadRows(at: [indexPath], with: .automatic)
      }
    case .downloaded:
      let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
      let alert = UIAlertController(
        title: Strings.PlayList.removePlaylistOfflineDataAlertTitle, message: Strings.PlayList.removePlaylistOfflineDataAlertMessage, preferredStyle: style)

      alert.addAction(
        UIAlertAction(
          title: Strings.PlayList.removeActionButtonTitle, style: .destructive,
          handler: { [unowned self] _ in
            _ = PlaylistManager.shared.deleteCache(item: item)
            self.tableView.reloadRows(at: [indexPath], with: .automatic)
          }))

      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }

  func deleteItem(itemId: String, indexPath: IndexPath) {
    let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
    let alert = UIAlertController(
      title: Strings.PlayList.removePlaylistVideoAlertTitle, message: Strings.PlayList.removePlaylistVideoAlertMessage, preferredStyle: style)

    alert.addAction(
      UIAlertAction(
        title: Strings.PlayList.removeActionButtonTitle, style: .destructive,
        handler: { [weak self] _ in
          guard let self = self else { return }

          self.delegate?.deleteItem(itemId: itemId, at: indexPath.row)

          if self.delegate?.currentPlaylistItem == nil {
            self.updateTableBackgroundView()
            self.activityIndicator.stopAnimating()
          }
        }))

    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
    self.present(alert, animated: true, completion: nil)
  }

  func openInNewTab(_ item: PlaylistInfo, isPrivate: Bool) {
    if let browser = PlaylistCarplayManager.shared.browserController,
      let pageURL = URL(string: item.pageSrc) {

      self.dismiss(animated: true) {
        browser.tabManager.addTabAndSelect(
          URLRequest(url: pageURL),
          isPrivate: isPrivate)
      }
    }
  }
  
  func openInPrivateTabWithAuthentication(_ item: PlaylistInfo) {
    if !isPrivateBrowsing, Preferences.Privacy.privateBrowsingLock.value {
      askForLocalAuthentication { [weak self] success, _ in
        if success {
          self?.openInNewTab(item, isPrivate: true)
        }
      }
    } else {
      self.openInNewTab(item, isPrivate: true)
    }
  }
  
}

// MARK: UITableViewDelegate

extension PlaylistListViewController: UITableViewDelegate {

  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    .none
  }

  func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
    false
  }

  func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {

    guard PlaylistManager.shared.currentFolder?.isPersistent == true,
          let currentItem = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
      return nil
    }
    
    let isSharedFolder = PlaylistManager.shared.currentFolder?.sharedFolderId != nil
    let cacheState = PlaylistManager.shared.state(for: currentItem.tagId)

    let cacheAction = UIContextualAction(
      style: .normal, title: nil,
      handler: { [weak self] (action, view, completionHandler) in
        self?.cacheItem(currentItem, indexPath: indexPath, cacheState: cacheState)
        completionHandler(true)
      })

    let deleteAction = UIContextualAction(
      style: .normal, title: nil,
      handler: { [weak self] (action, view, completionHandler) in
        self?.deleteItem(itemId: currentItem.tagId, indexPath: indexPath)
        completionHandler(true)
      })

    let shareAction = UIContextualAction(
      style: .normal, title: nil,
      handler: { [weak self] (action, view, completionHandler) in
        guard let self = self else { return }

        let isPrivateBrowsing = self.isPrivateBrowsing
        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet

        let alert = UIAlertController(
          title: currentItem.pageTitle,
          message: nil,
          preferredStyle: style)

        // If we're already in private browsing, this should not show
        // Option to open in regular tab
        if !isPrivateBrowsing {
          alert.addAction(
            UIAlertAction(
              title: Strings.PlayList.sharePlaylistOpenInNewTabTitle, style: .default,
              handler: { [weak self] _ in
                self?.openInNewTab(currentItem, isPrivate: false)
              }))
        }

        // Option to open in private browsing tab
        alert.addAction(
          UIAlertAction(
            title: Strings.PlayList.sharePlaylistOpenInNewPrivateTabTitle, style: .default,
            handler: { [weak self] _ in
              self?.openInPrivateTabWithAuthentication(currentItem)
            }))

        if !isSharedFolder {
          alert.addAction(
            UIAlertAction(
              title: Strings.PlayList.sharePlaylistMoveActionMenuTitle, style: .default,
              handler: { [weak self] _ in
                self?.moveItems(indexPaths: [indexPath])
              }))
        }

        alert.addAction(
          UIAlertAction(
            title: Strings.PlayList.sharePlaylistShareActionMenuTitle, style: .default,
            handler: { [weak self] _ in
              self?.shareItem(currentItem, anchorView: tableView.cellForRow(at: indexPath))
            }))

        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        self.present(alert, animated: true, completion: nil)

        completionHandler(true)
      })

    cacheAction.image = cacheState == .invalid ? UIImage(braveSystemNamed: "leo.cloud.download")! : UIImage(braveSystemNamed: "leo.cloud.off")!
    cacheAction.backgroundColor = UIColor.braveDarkerBlurple

    deleteAction.image = UIImage(braveSystemNamed: "leo.trash")!
    deleteAction.backgroundColor = UIColor.braveErrorLabel

    shareAction.image = UIImage(systemName: "square.and.arrow.up")
    shareAction.backgroundColor = UIColor.braveInfoLabel

    if isSharedFolder {
      return UISwipeActionsConfiguration(actions: [shareAction, cacheAction])
    }
    return UISwipeActionsConfiguration(actions: [deleteAction, shareAction, cacheAction])
  }

  func tableView(_ tableView: UITableView, contextMenuConfigurationForRowAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
    guard PlaylistManager.shared.currentFolder?.isPersistent == true,
          let currentItem = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
      return nil
    }
    
    let isSharedFolder = PlaylistManager.shared.currentFolder?.sharedFolderId != nil

    let actionProvider: UIContextMenuActionProvider = { _ in
      let cacheState = PlaylistManager.shared.state(for: currentItem.tagId)
      let cacheTitle = cacheState == .invalid ? Strings.PlayList.playlistSaveForOfflineButtonTitle : Strings.PlayList.playlistDeleteForOfflineButtonTitle
      let cacheIcon = cacheState == .invalid ? UIImage(braveSystemNamed: "leo.cloud.download") : UIImage(braveSystemNamed: "leo.cloud.off")
      
      var menuItems: [UIMenuElement] = [
        UIMenu(
          options: .displayInline,
          children: [
            UIAction(
              title: cacheTitle, image: cacheIcon,
              handler: { [weak self] _ in
                self?.cacheItem(currentItem, indexPath: indexPath, cacheState: cacheState)
              })
          ]),

        UIMenu(
          options: .displayInline,
          children: {
            var actions = [UIMenuElement]()

            // In Private-Browsing, we do not show "Open in New Tab",
            // we only show "Open in Private Tab"
            let isPrivateBrowsing = self.isPrivateBrowsing
            if !isPrivateBrowsing {
              actions.append(
                UIAction(
                  title: Strings.PlayList.sharePlaylistOpenInNewTabTitle, image: UIImage(systemName: "plus.square.on.square"),
                  handler: { [weak self] _ in
                    self?.openInNewTab(currentItem, isPrivate: false)
                  }))
            }

            actions.append(
              UIAction(
                title: Strings.PlayList.sharePlaylistOpenInNewPrivateTabTitle, image: UIImage(systemName: "plus.square.fill.on.square.fill"),
                handler: { [weak self] _ in
                  self?.openInPrivateTabWithAuthentication(currentItem)
                }))

            return actions
          }())
      ]
      
      if !isSharedFolder {
        menuItems += [
          UIMenu(
            options: .displayInline,
            children: {
              var actions = [UIMenuElement]()
              if PlaylistFolder.getOtherFoldersCount() > 0 {
                actions.append(
                  UIAction(
                    title: Strings.PlayList.sharePlaylistMoveActionMenuTitle, image: UIImage(braveSystemNamed: "leo.folder"),
                    handler: { [weak self] _ in
                      self?.moveItems(indexPaths: [indexPath])
                    }))
              }

              actions.append(
                UIAction(
                  title: Strings.PlayList.sharePlaylistShareActionMenuTitle, image: UIImage(systemName: "square.and.arrow.up"),
                  handler: { [weak self] _ in
                    self?.shareItem(currentItem, anchorView: tableView.cellForRow(at: indexPath))
                  }))

              return actions
            }()),

          UIAction(
            title: Strings.delete, image: UIImage(braveSystemNamed: "leo.trash"), attributes: .destructive,
            handler: { [weak self] _ in
              self?.deleteItem(itemId: currentItem.tagId, indexPath: indexPath)
            })
        ]
      }

      return UIMenu(children: menuItems)
    }

    let identifier = NSDictionary(dictionary: [
      "itemID": currentItem.tagId,
      "row": indexPath.row,
    ])
    return UIContextMenuConfiguration(identifier: identifier, previewProvider: nil, actionProvider: actionProvider)
  }

  func tableView(_ tableView: UITableView, previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    guard let identifier = configuration.identifier as? NSDictionary else {
      return nil
    }

    guard let itemID = identifier["itemID"] as? String,
      let row = identifier["row"] as? Int
    else {
      return nil
    }

    guard row >= 0 || row < PlaylistManager.shared.numberOfAssets,
      let currentItem = PlaylistManager.shared.itemAtIndex(row),
      currentItem.tagId == itemID
    else {
      return nil
    }

    guard let cell = tableView.cellForRow(at: IndexPath(row: row, section: 0)) else {
      return nil
    }

    let parameters = UIPreviewParameters()
    parameters.visiblePath = UIBezierPath(roundedRect: cell.contentView.frame, cornerRadius: 12.0)
    parameters.backgroundColor = .clear  // If we set this to any other colour, UIKit renders white lol

    return UITargetedPreview(view: cell, parameters: parameters)
  }

  func tableView(_ tableView: UITableView, previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    self.tableView(tableView, previewForHighlightingContextMenuWithConfiguration: configuration)
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if tableView.isEditing {
      updateToolbar(editing: true)
      return
    }
    
    delegate?.showStaticImage(image: nil)

    prepareToPlayItem(at: indexPath) { [weak self] item in
      guard let self = self, let item = item else {
        self?.activityIndicator.stopAnimating()
        return
      }

      PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
      PlaylistCarplayManager.shared.currentPlaylistItem = item
      
      PlaylistManager.shared.playbackTask = Task { @MainActor in
        do {
          guard let item = try await self.delegate?.playItem(item: item) else {
            self.activityIndicator.stopAnimating()
            return
          }
          
          PlaylistCarplayManager.shared.currentPlaylistItem = item
          self.activityIndicator.stopAnimating()
          
          PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
          PlaylistCarplayManager.shared.currentPlaylistItem = item
          self.commitPlayerItemTransaction(at: indexPath, isExpired: false)
          
          self.seekLastPlayedItem(
            at: indexPath,
            lastPlayedItemId: item.tagId,
            lastPlayedTime: item.lastPlayedOffset)
        } catch {
          PlaylistCarplayManager.shared.currentPlaylistItem = nil
          PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
          self.commitPlayerItemTransaction(at: indexPath, isExpired: false)
          self.delegate?.displayLoadingResourceError()
          Logger.module.error("Playlist Playback Error: \(error)")
        }
      }
    }
  }

  func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
    if tableView.isEditing {
      updateToolbar(editing: true)
    }
  }
}
