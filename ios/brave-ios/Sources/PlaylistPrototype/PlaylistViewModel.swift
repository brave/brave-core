// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Data
import Foundation
import UIKit

class PlaylistViewModel: ObservableObject {
  @Published var items: [PlaylistItem] = []
  @Published var folders: [PlaylistFolder] = []

  private let itemsFRCDelegate = FetchedResultControllerDelegate()
  private let foldersFRCDelegate = FetchedResultControllerDelegate()

  init() {
    self.itemsFRC = PlaylistItem.frc()
    self.foldersFRC = PlaylistFolder.frc(savedFolder: true, sharedFolders: false)

    itemsFRC.delegate = itemsFRCDelegate
    foldersFRC.delegate = foldersFRCDelegate

    itemsFRCDelegate.changed = { [weak self] in
      self?.fetchItems()
    }
    foldersFRCDelegate.changed = { [weak self] in
      self?.fetchFolders()
    }
  }

  private let itemsFRC: NSFetchedResultsController<PlaylistItem>
  private let foldersFRC: NSFetchedResultsController<PlaylistFolder>

  func fetchFolders() {
    do {
      try foldersFRC.performFetch()
      if let objects = foldersFRC.fetchedObjects {
        folders = objects
      }
    } catch {
      print("Failed to load objects")
    }
  }

  func fetchItems() {
    do {
      try itemsFRC.performFetch()
      if let objects = itemsFRC.fetchedObjects {
        items = objects
      }
    } catch {
      print("Failed to load objects")
    }
  }

  class FetchedResultControllerDelegate: NSObject, NSFetchedResultsControllerDelegate {
    var changed: (() -> Void)?

    func controller(
      _ controller: NSFetchedResultsController<any NSFetchRequestResult>,
      didChangeContentWith snapshot: NSDiffableDataSourceSnapshotReference
    ) {
      changed?()
    }
  }
}
