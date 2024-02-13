// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SwiftUI
import AVKit
import AVFoundation
import Data
import Shared
import BraveUI
import Preferences
import Favicon
import os.log
import Playlist

// MARK: UITableViewDataSource

extension PlaylistListViewController: UITableViewDataSource {
  private static let formatter = DateComponentsFormatter().then {
    $0.allowedUnits = [.day, .hour, .minute, .second]
    $0.unitsStyle = .abbreviated
    $0.maximumUnitCount = 2
  }

  func getAssetDurationFormatted(item: PlaylistInfo, _ completion: @escaping (String) -> Void) {
    PlaylistManager.shared.getAssetDuration(item: item) { duration in
      let domain = URL(string: item.pageSrc)?.baseDomain ?? "0\(Strings.Shields.shieldsTimeStatsSeconds)"
      if let duration = duration {
        if duration.isInfinite {
          // Live video/audio
          completion(Strings.PlayList.playlistLiveMediaStream)
        } else if abs(duration.distance(to: 0.0)) > 0.00001 {
          completion(Self.formatter.string(from: duration) ?? domain)
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

  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    PlaylistManager.shared.currentFolder?.isPersistent == true
  }

  func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
    PlaylistManager.shared.currentFolder?.sharedFolderId == nil && !tableView.isEditing
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    loadingState == .loading ? Constants.tableRedactedCellCount : PlaylistManager.shared.numberOfAssets
  }

  func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    UITableView.automaticDimension
  }
  
  func tableView(_ tableView: UITableView, estimatedHeightForRowAt indexPath: IndexPath) -> CGFloat {
    Constants.tableRowHeight
  }

  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    UITableView.automaticDimension
  }
  
  func tableView(_ tableView: UITableView, estimatedHeightForHeaderInSection section: Int) -> CGFloat {
    Constants.tableHeaderHeight
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    // Display Redacted Cells
    if loadingState != .fullyLoaded {
      guard let cell = tableView.dequeueReusableCell(withIdentifier: Constants.playlistCellRedactedIdentifier, for: indexPath) as? PlaylistCellRedacted else {
        return UITableViewCell()
      }
      
      guard indexPath.row >= 0,
            PlaylistManager.shared.currentFolder != nil,
            let item = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
        cell.do {
          $0.showsReorderControl = false
          $0.setTitle(title: nil)
          $0.setDetails(details: nil)
          $0.setContentSize(parentController: self)
        }
        return cell
      }
      
      cell.do {
        $0.showsReorderControl = false
        $0.setTitle(title: item.name)
        $0.setDetails(details: URL(string: item.pageSrc)?.baseDomain ?? "0\(Strings.Shields.shieldsTimeStatsSeconds)")
        $0.setContentSize(parentController: self)
        
        if let url = URL(string: item.pageSrc) {
          $0.loadThumbnail(for: url)
        }
      }

      return cell
    }
    
    // Display Item Cells
    guard let cell = tableView.dequeueReusableCell(withIdentifier: Constants.playListCellIdentifier, for: indexPath) as? PlaylistCell else {
      return UITableViewCell()
    }
    
    guard let item = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
      return cell
    }

    cell.prepareForDisplay()

    cell.do {
      $0.showsReorderControl = false
      $0.titleLabel.text = item.name
      $0.detailLabel.text = URL(string: item.pageSrc)?.baseDomain ?? "0\(Strings.Shields.shieldsTimeStatsSeconds)"
      $0.contentView.backgroundColor = .clear
      $0.backgroundColor = .clear
      $0.iconView.image = nil
      $0.iconView.backgroundColor = .black
      $0.selectedBackgroundView = UIView().then {
        $0.backgroundColor = .tertiaryBraveBackground
      }
    }

    let cacheState = PlaylistManager.shared.state(for: item.tagId)
    self.updateCellDownloadStatus(
      indexPath: indexPath,
      cell: cell,
      state: cacheState,
      percentComplete: nil)

    // Load the HLS/Media thumbnail. If it fails, fall-back to favIcon
    if let assetUrl = URL(string: item.src), let favIconUrl = URL(string: item.pageSrc) {
      cell.loadingView.startAnimating()
      cell.thumbnailGenerator.loadThumbnail(assetUrl: assetUrl, favIconUrl: favIconUrl) { [weak cell] image in
        guard let cell = cell else { return }

        cell.iconView.image = image ?? Favicon.defaultImage
        cell.iconView.backgroundColor = .black
        cell.iconView.contentMode = .scaleAspectFit
        cell.loadingView.stopAnimating()
      }
    }

    return cell
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let folder = PlaylistManager.shared.currentFolder
    
    if loadingState != .fullyLoaded {
      let header = tableView.dequeueReusableHeaderFooterView(withIdentifier: Constants.playListMenuHeaderRedactedIdentifier) as? PlaylistRedactedHeader
      header?.setTitle(title: folder?.title)
      header?.setCreatorName(creatorName: folder?.creatorName)
      return header
    }
    
    guard let folder = folder, folder.playlistItems?.isEmpty == false else {
      return nil
    }
    
    let isPersistent = folder.isPersistent
    let header = tableView.dequeueReusableHeaderFooterView(withIdentifier: Constants.playListMenuHeaderIdentifier) as? PlaylistMenuHeader
    return header?.then { header in
      header.titleLabel.text = folder.title
      header.subtitleLabel.text = folder.creatorName
      header.subtitleLabel.isHidden = folder.creatorName == nil
      header.setState(isPersistent ? .menu : .add)
      
      header.onAddPlaylist = { [unowned self] in
        guard let sharedFolderUrl = folder.sharedFolderUrl else { return }
        
        if PlayListDownloadType(rawValue: Preferences.Playlist.autoDownloadVideo.value) != .off {
          let controller = PopupViewController(rootView: PlaylistFolderSharingManagementView(onAddToPlaylistPressed: { [unowned self] in
            self.dismiss(animated: true)
            
            Task { @MainActor in
              let persistentFolderId = await PlaylistSharedFolderNetwork.saveToDiskStorage(memoryFolder: folder)
              PlaylistManager.shared.currentFolder = PlaylistFolder.getFolder(uuid: persistentFolderId)
              
              PlaylistFolder.getSharedFolder(sharedFolderUrl: sharedFolderUrl)?.playlistItems?.forEach({
                PlaylistManager.shared.autoDownload(item: PlaylistInfo(item: $0))
              })
            }
          }, onSettingsPressed: { [unowned self] in
            let delegate = self.delegate
            self.dismiss(animated: false) {
              delegate?.openPlaylistSettings()
            }
          }, onCancelPressed: { [unowned self] in
            self.dismiss(animated: true)
          })).then {
            $0.overrideUserInterfaceStyle = .dark
          }
          
          self.present(controller, animated: true, completion: nil)
        } else {
          Task { @MainActor in
            let persistentFolderId = await PlaylistSharedFolderNetwork.saveToDiskStorage(memoryFolder: folder)
            PlaylistManager.shared.currentFolder = PlaylistFolder.getFolder(uuid: persistentFolderId)
          }
        }
      }
      
      header.menu = { [weak header, weak folder] in
        guard isPersistent,
              let folder = folder,
              let folderId = folder.uuid
        else { return nil }
        
        let syncAction = UIAction(title: Strings.PlaylistFolderSharing.syncNowMenuTitle, image: UIImage(braveSystemNamed: "leo.refresh")?.template) { _ in
          guard let sharedFolderUrl = folder.sharedFolderUrl else {
            Logger.module.error("Invalid Playlist Shared Folder URL")
            return
          }
          
          Task { @MainActor in
            do {
              try await PlaylistManager.syncSharedFolder(sharedFolderUrl: sharedFolderUrl)
              PlaylistManager.shared.currentFolder = PlaylistFolder.getFolder(uuid: folderId)
            } catch {
              if let error = error as? PlaylistSharedFolderNetwork.Status {
                Logger.module.error("\(error.localizedDescription)")
              } else {
                Logger.module.error("CANNOT SYNC SHARED PLAYLIST: \(error.localizedDescription)")
              }
            }
          }
        }
        
        let editAction = UIAction(title: Strings.PlaylistFolderSharing.editMenuTitle, image: UIImage(braveSystemNamed: "leo.edit.pencil")?.template) { [unowned self] _ in
          self.onEditItems()
        }
        
        let renameAction = UIAction(title: Strings.PlaylistFolderSharing.renameMenuTitle, image: UIImage(braveSystemNamed: "leo.folder.text")?.template) { [unowned self] _ in
          let folderID = folder.objectID
          var editView = PlaylistEditFolderView(currentFolder: folderID, currentFolderTitle: folder.title ?? "")

          editView.onCancelButtonPressed = { [unowned self] in
            self.presentedViewController?.dismiss(animated: true, completion: nil)
          }

          editView.onEditFolder = { [unowned self] folderTitle in
            PlaylistFolder.updateFolder(folderID: folderID) { [weak self] result in
              guard let self = self else { return }
              
              switch result {
              case .failure(let error):
                Logger.module.error("Error Saving Folder Title: \(error.localizedDescription)")

                DispatchQueue.main.async {
                  let alert = UIAlertController(title: Strings.genericErrorTitle, message: Strings.PlaylistFolders.playlistFolderErrorSavingMessage, preferredStyle: .alert)

                  alert.addAction(
                    UIAlertAction(
                      title: Strings.OBErrorOkay, style: .default,
                      handler: { _ in
                        self.presentedViewController?.dismiss(animated: true, completion: nil)
                      }))
                  self.present(alert, animated: true, completion: nil)
                }

              case .success(let folder):
                folder.title = folderTitle
                
                DispatchQueue.main.async {
                  header?.titleLabel.text = folderTitle
                  self.title = folderTitle
                  self.presentedViewController?.dismiss(animated: true, completion: nil)
                }
              }
            }
          }

          let hostingController = UIHostingController(rootView: editView.environment(\.managedObjectContext, folder.managedObjectContext ?? DataController.swiftUIContext)).then {
            $0.modalPresentationStyle = .formSheet
          }

          self.present(hostingController, animated: true, completion: nil)
        }
        
        var canSaveOffline = true
        for item in folder.playlistItems ?? [] {
          if let cachedData = item.cachedData, !cachedData.isEmpty {
            canSaveOffline = false
          }
        }
        
        let saveOfflineAction = UIAction(title: Strings.PlaylistFolderSharing.saveOfflineDataMenuTitle, image: UIImage(braveSystemNamed: "leo.cloud.download")?.template) { [unowned self] _ in
          folder.playlistItems?.forEach {
            PlaylistManager.shared.download(item: PlaylistInfo(item: $0))
          }
          
          self.tableView.reloadData()
        }
        
        let deleteOfflineAction = UIAction(title: Strings.PlaylistFolderSharing.deleteOfflineDataMenuTitle, image: UIImage(braveSystemNamed: "leo.cloud.off")?.template) { [unowned self] _ in
          folder.playlistItems?.forEach {
            PlaylistManager.shared.deleteCache(item: PlaylistInfo(item: $0))
          }
          
          self.tableView.reloadData()
        }
        
        let deleteAction = UIAction(title: Strings.PlaylistFolderSharing.deletePlaylistMenuTitle, image: UIImage(braveSystemNamed: "leo.trash")?.template, attributes: .destructive) { [unowned self] _ in
          
          PlaylistManager.shared.delete(folder: folder) { success in
            if success {
              self.navigationController?.popToRootViewController(animated: true)
            }
          }
        }
        
        if folder.sharedFolderId != nil {
          return UIMenu(children: [syncAction, renameAction, canSaveOffline ? saveOfflineAction : deleteOfflineAction, deleteAction])
        }
        
        if folder.uuid == PlaylistFolder.savedFolderUUID {
          return UIMenu(children: [editAction, canSaveOffline ? saveOfflineAction : deleteOfflineAction])
        }
        
        return UIMenu(children: [editAction, renameAction, canSaveOffline ? saveOfflineAction : deleteOfflineAction, deleteAction])
      }()
    }
  }
}
