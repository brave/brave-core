// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Foundation
import Playlist
import Preferences
import TestHelpers
import XCTest

@testable import Data
@testable import PlaylistUI

class PlayerModelTests: CoreDataTestCase {

  override func setUp() async throws {
    try await super.setUp()

    Preferences.Playlist.firstLoadAutoPlay.value = false
    Preferences.Playlist.lastPlayedItemUrl.value = nil
    Preferences.Playlist.playbackLeftOff.value = false
  }

  private func addFolder() async throws -> PlaylistFolder {
    return try await withCheckedThrowingContinuation { continuation in
      PlaylistFolder.addFolder(title: "Folder") { id in
        guard let folder = PlaylistFolder.getFolder(uuid: id) else {
          continuation.resume(throwing: NSError())
          return
        }
        continuation.resume(returning: folder)
      }
    }
  }

  @MainActor private func addMockItems(count: Int = 10, to folder: PlaylistFolder) async {
    let infos: [PlaylistInfo] = (0..<count).map { i in
      .init(
        name: "Item \(i)",
        src: "https://\(i).com",
        pageSrc: "blob://\(i)",
        pageTitle: "Title \(i)",
        mimeType: "",
        duration: 0.0,
        lastPlayedOffset: 0,
        detected: false,
        dateAdded: .now,
        tagId: UUID().uuidString,
        order: Int32(i),
        isInvisible: false
      )
    }
    let testVideoData = try! Bundle.module.url(
      forResource: "Big_Buck_Bunny_360_10s_1MB",
      withExtension: "mp4"
    )!.bookmarkData()
    await withCheckedContinuation { continuation in
      PlaylistItem.addItems(infos, folderUUID: folder.uuid, cachedData: testVideoData) {
        continuation.resume()
      }
    }
  }

  // MARK: - Queue

  /// Tests the empty state for when there are no items in the queue, the selected folder should
  /// default to the predefined "play later" folder
  @MainActor func testEmptyQueue() async throws {
    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedFolderID, PlaylistFolder.savedFolderUUID)
    XCTAssertNil(playerModel.selectedItemID)
  }

  /// Tests basic queue construction to ensure ordering follows order & date added
  @MainActor func testStandardQueueOrdering() async throws {
    let folder = try await addFolder()
    await addMockItems(to: folder)

    let itemsIDs = PlaylistItem.getItems(parentFolder: folder).map(\.id)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    XCTAssertEqual(Array(playerModel.itemQueue), itemsIDs)
  }

  /// Tests the queue when a given item is selected which shouldn't affect the queue since a user
  /// could go back and forth anyways
  @MainActor func testItemSelectedQueueOrdering() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let itemsIDs = PlaylistItem.getItems(parentFolder: folder).map(\.id)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    playerModel.selectedItemID = itemsIDs[4]
    await playerModel.prepareItemQueue()

    XCTAssertEqual(Array(playerModel.itemQueue), itemsIDs)
  }

  /// Tests that shuffle mode appropriately shuffles the queue and when there is a selected item
  /// places said item at the top of the qeuue
  @MainActor func testShuffledQueueOrdering() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 100, to: folder)

    let itemsIDs = PlaylistItem.getItems(parentFolder: folder).map(\.id)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()
    XCTAssertEqual(Array(playerModel.itemQueue), itemsIDs)

    // Setting shuffle is enabled should re-make the queue
    playerModel.isShuffleEnabled = true

    XCTAssertNotEqual(Array(playerModel.itemQueue), itemsIDs)

    // Making an shuffled item queue with a selected item should insert that item at the start
    // of the queue
    playerModel.makeItemQueue(selectedItemID: itemsIDs[5])
    XCTAssertEqual(playerModel.itemQueue.first, itemsIDs[5])
  }

  /// Tests that if you create a player model with initial playback info, that initial playback info
  /// is used to automatically select an item
  @MainActor func testPreparingWithInitialPlaybackInfo() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)
    let initialPlaybackInfo: PlayerModel.InitialPlaybackInfo = .init(
      itemUUID: try XCTUnwrap(items[5].uuid),
      timestamp: 0
    )

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: initialPlaybackInfo)
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, items[5].id)
    XCTAssertEqual(playerModel.selectedFolderID, try XCTUnwrap(items[5].playlistFolder).id)
  }

  /// Tests that if you create a player model with initial playback info and last played item info,
  /// that initial playback info should be used to automatically select an item over the last played
  /// item.
  @MainActor func testPreparingWithInitialPlaybackInfoAndLastPlayedItem() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)
    let initialPlaybackInfo: PlayerModel.InitialPlaybackInfo = .init(
      itemUUID: try XCTUnwrap(items[5].uuid),
      timestamp: 0
    )

    Preferences.Playlist.lastPlayedItemUrl.value = items[6].pageSrc

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: initialPlaybackInfo)
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, items[5].id)
    XCTAssertEqual(playerModel.selectedFolderID, try XCTUnwrap(items[5].playlistFolder).id)
  }

  /// Tests playing the next item in the queue with repeat mode off
  @MainActor func testPlayNextItemRepeatModeOff() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let itemsIDs = PlaylistItem.getItems(parentFolder: folder).map(\.id)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    playerModel.repeatMode = .none
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.itemQueue.first, itemsIDs.first)
    XCTAssertEqual(playerModel.selectedItemID, itemsIDs.first)

    await playerModel.playNextItem()

    XCTAssertEqual(playerModel.selectedItemID, itemsIDs[1])
  }

  /// Tests playing the next item in the queue with repeat mode off while playing the final item in
  /// the queue
  @MainActor func testPlayNextItemLastItemRepeatModeOff() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    let playerModel = PlayerModel(
      mediaStreamer: nil,
      initialPlaybackInfo: .init(
        itemUUID: try XCTUnwrap(items[9].uuid),
        timestamp: 0
      )
    )
    playerModel.repeatMode = .none
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, items[9].id)
    XCTAssertFalse(playerModel.canPlayNextItem)

    await playerModel.playNextItem()

    // Playing the final item doesn't deselect the item, just pauses it.
    XCTAssertEqual(playerModel.selectedItemID, items[9].id)
  }

  /// Tests playing the next item in the queue with repeat mode set to one item
  @MainActor func testPlayNextItemRepeatModeOne() async throws {
    let folder = try await addFolder()
    await addMockItems(to: folder)

    let itemsIDs = PlaylistItem.getItems(parentFolder: folder).map(\.id)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()
    playerModel.repeatMode = .one

    XCTAssertEqual(playerModel.itemQueue.first, itemsIDs.first)
    XCTAssertEqual(playerModel.selectedItemID, itemsIDs.first)

    await playerModel.playNextItem()

    XCTAssertEqual(playerModel.selectedItemID, itemsIDs.first)
    XCTAssertTrue(playerModel.canPlayNextItem)

    await playerModel.playNextItem()

    XCTAssertEqual(playerModel.selectedItemID, itemsIDs.first)
  }

  /// Tests playing the next item in the queue with repeat mode set to all items
  @MainActor func testPlayNextItemRepeatModeAll() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    let playerModel = PlayerModel(
      mediaStreamer: nil,
      initialPlaybackInfo: .init(
        itemUUID: try XCTUnwrap(items[8].uuid),
        timestamp: 0
      )
    )
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()
    playerModel.repeatMode = .all

    XCTAssertEqual(playerModel.selectedItemID, items[8].id)
    XCTAssertTrue(playerModel.canPlayNextItem)

    await playerModel.playNextItem()

    XCTAssertEqual(playerModel.selectedItemID, items[9].id)
    XCTAssertTrue(playerModel.canPlayNextItem)

    await playerModel.playNextItem()

    XCTAssertEqual(playerModel.selectedItemID, items.first?.id)
  }

  /// Tests playing the previous item from the start of the queue which should just restart
  /// the current item
  @MainActor func testPlayPreviousItemFromStartOfQueue() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let itemsIDs = PlaylistItem.getItems(parentFolder: folder).map(\.id)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, itemsIDs.first)

    await playerModel.seek(to: 1, accurately: true)
    XCTAssertNotEqual(playerModel.currentTime, 0)

    await playerModel.playPreviousItem()
    XCTAssertEqual(playerModel.selectedItemID, itemsIDs.first)
    XCTAssertEqual(playerModel.currentTime, 0)
  }

  /// Tests playing the previous item successfully from somewhere other than the first item in the
  /// queue
  @MainActor func testPlayPreviousItem() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    let playerModel = PlayerModel(
      mediaStreamer: nil,
      initialPlaybackInfo: .init(
        itemUUID: try XCTUnwrap(items[1].uuid),
        timestamp: 0
      )
    )
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, items[1].id)

    await playerModel.playPreviousItem()
    XCTAssertEqual(playerModel.selectedItemID, items.first?.id)
  }

  /// Tests that if there is a last played item it is pre-selected upon preparing the item queue
  @MainActor func testLastPlayedItemPreference() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    Preferences.Playlist.lastPlayedItemUrl.value = items[5].pageSrc

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, items[5].id)
    XCTAssertEqual(playerModel.selectedFolderID, try XCTUnwrap(items[5].playlistFolder).id)
  }

  /// Tests that we resume playback of an item based on where the user last played
  @MainActor func testResumingPlaybackFromLastPlayedOffset() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 1, to: folder)

    Preferences.Playlist.playbackLeftOff.value = true

    let item = try XCTUnwrap(PlaylistItem.getItems(parentFolder: folder).first)
    await withCheckedContinuation { continuation in
      item.lastPlayedOffset = 3
      PlaylistItem.updateItem(.init(item: item)) {
        continuation.resume()
      }
    }

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.currentTime, 3)
  }

  /// Tests that playback is correctly saved when the app resigns being active
  @MainActor func testSavingPlaybackOnAppResign() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 1, to: folder)

    Preferences.Playlist.playbackLeftOff.value = true

    let item = try XCTUnwrap(PlaylistItem.getItems(parentFolder: folder).first)
    XCTAssertEqual(item.lastPlayedOffset, 0)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.currentTime, 0)
    await playerModel.seek(to: 3, accurately: true)
    XCTAssertEqual(playerModel.currentTime, 3)

    NotificationCenter.default.post(name: UIApplication.willResignActiveNotification, object: nil)

    let mergeExpectation = expectation(description: "merge")
    let saveExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    ) { notification in
      DispatchQueue.main.async {
        DataController.viewContext.mergeChanges(fromContextDidSave: notification)
        mergeExpectation.fulfill()
      }
      return true
    }

    await fulfillment(of: [mergeExpectation, saveExpectation], timeout: 1)

    let lastPlayedOffset = PlaylistItem.getItem(id: item.id)?.lastPlayedOffset
    XCTAssertEqual(lastPlayedOffset, 3)
  }

  /// Test having both an initial offset passed in and the playbackLeftOff preference being enabled
  @MainActor func testResumingPlaybackWithBothLastPlayedOffsetAndInitialItem() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    Preferences.Playlist.playbackLeftOff.value = true

    let items = PlaylistItem.getItems(parentFolder: folder)
    await withCheckedContinuation { continuation in
      items[0].lastPlayedOffset = 3
      PlaylistItem.updateItem(.init(item: items[0])) {
        continuation.resume()
      }
    }

    let initialItemOffset: TimeInterval = 2
    let playerModel = PlayerModel(
      mediaStreamer: nil,
      initialPlaybackInfo: .init(
        itemUUID: try XCTUnwrap(items[1].uuid),
        timestamp: initialItemOffset
      )
    )
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    XCTAssertEqual(playerModel.selectedItemID, items[1].id)
    // Initial item takes precedence
    XCTAssertEqual(playerModel.currentTime, initialItemOffset)
  }

  // MARK: - Playback

  /// Tests basic item playback controls such as play, pause, seek
  @MainActor func testBasicItemPlaybackControls() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 1, to: folder)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    defer {
      playerModel.stop()
    }

    // Required for the player to actually play immediately in unit tests
    playerModel.playerLayer.player?.automaticallyWaitsToMinimizeStalling = false

    playerModel.play()
    XCTAssertTrue(playerModel.isPlaying)

    await playerModel.seek(to: 5, accurately: true)
    XCTAssertEqual(playerModel.currentTime.rounded(), 5)

    playerModel.pause()
    XCTAssertFalse(playerModel.isPlaying)
  }

  /// Tests the sleep timer pausing playback when the date condition passes
  @MainActor func testSleepTimerDateCondition() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()

    defer {
      playerModel.stop()
    }

    // Required for the player to actually play immediately in unit tests
    playerModel.playerLayer.player?.automaticallyWaitsToMinimizeStalling = false

    playerModel.play()
    XCTAssertTrue(playerModel.isPlaying)

    playerModel.sleepTimerCondition = .date(.now + 0.25)
    try await Task.sleep(for: .milliseconds(300))

    XCTAssertFalse(playerModel.isPlaying)
  }

  /// Tests the sleep timer pausing playback when item playback completion condition passes
  @MainActor func testSleepTimerEndOfItemCondition() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 10, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    await playerModel.prepareItemQueue()
    XCTAssertEqual(playerModel.selectedItemID, playerModel.itemQueue.first)

    defer {
      playerModel.stop()
    }

    // Required for the player to actually play immediately in unit tests
    playerModel.playerLayer.player?.automaticallyWaitsToMinimizeStalling = false

    XCTAssertFalse(playerModel.isPlaying)
    playerModel.play()
    XCTAssertTrue(playerModel.isPlaying)

    playerModel.sleepTimerCondition = .itemPlaybackCompletion

    let didPlayToEndTime = expectation(
      forNotification: AVPlayerItem.didPlayToEndTimeNotification,
      object: nil
    )
    await playerModel.seek(to: try XCTUnwrap(playerModel.duration.seconds) - 0.1, accurately: true)
    await fulfillment(of: [didPlayToEndTime], timeout: 1)

    XCTAssertFalse(playerModel.isPlaying)
    XCTAssertEqual(playerModel.selectedItemID, playerModel.itemQueue.first)
  }

  /// Tests that the player model immediately plays the item when the auto play preference is true
  @MainActor func testAutoPlay() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 1, to: folder)

    Preferences.Playlist.firstLoadAutoPlay.value = true

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    // Required for the player to actually play immediately in unit tests
    playerModel.playerLayer.player?.automaticallyWaitsToMinimizeStalling = false
    await playerModel.prepareItemQueue()

    defer {
      playerModel.stop()
    }

    XCTAssertTrue(playerModel.isPlaying)
    XCTAssertEqual(playerModel.selectedItemID, playerModel.itemQueue.first)
  }

  @MainActor func testDurationAvailableOnLoad() async throws {
    let folder = try await addFolder()
    await addMockItems(count: 1, to: folder)

    let items = PlaylistItem.getItems(parentFolder: folder)

    let playerModel = PlayerModel(mediaStreamer: nil, initialPlaybackInfo: nil)
    playerModel.selectedFolderID = folder.id
    playerModel.selectedItemID = items[0].id
    // Required for the player to actually play immediately in unit tests
    playerModel.playerLayer.player?.automaticallyWaitsToMinimizeStalling = false
    await playerModel.prepareToPlaySelectedItem(initialOffset: 0, playImmediately: false)

    let status = playerModel.playerForTesting.currentItem?.asset.status(of: .duration)
    switch status {
    case .loaded:
      break
    default:
      XCTFail("Duration should already be loaded, but is currently \(String(describing: status))")
    }
  }
}
