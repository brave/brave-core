// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import XCTest

@testable import BraveTransferEngine

final class ChainSnapshotTests: XCTestCase {
  // The shape of BraveSyncAPI.getDeviceListJSON(): DeviceInfo::ToValue()
  // plus guid, isCurrentDevice and supportsSelfDelete.
  private let deviceListJSON = """
    [
      {"name": "James's iPhone", "id": "pub-1", "os": "ios", "type": "phone",
       "chromeVersion": "1.100", "lastUpdatedTimestamp": 1, "guid": "phone-guid",
       "sendTabToSelfReceivingEnabled": true, "hasSharingInfo": false,
       "isCurrentDevice": true, "supportsSelfDelete": true},
      {"name": "MacBook", "id": "pub-2", "os": "mac", "type": "desktop_or_laptop",
       "guid": "mac-guid", "isCurrentDevice": false},
      {"name": "iPad", "type": "tablet", "guid": "ipad-guid", "isCurrentDevice": false},
      {"name": "No guid", "type": "phone", "isCurrentDevice": false},
      {"name": "Odd", "type": "tv", "guid": "odd-guid"}
    ]
    """

  func testParsesDeviceList() throws {
    let snapshot = try XCTUnwrap(
      ChainSnapshot.fromSyncDeviceList(json: Data(deviceListJSON.utf8), isSyncActive: true))
    XCTAssertTrue(snapshot.isSyncActive)
    XCTAssertEqual(snapshot.localDeviceGUID, "phone-guid")
    XCTAssertEqual(snapshot.localDeviceName, "James's iPhone")
    XCTAssertEqual(
      snapshot.devices,
      [
        ChainDevice(guid: "mac-guid", name: "MacBook", type: .desktop),
        ChainDevice(guid: "ipad-guid", name: "iPad", type: .tablet),
        ChainDevice(guid: "odd-guid", name: "Odd", type: .unknown),
      ])
  }

  func testRejectsBadJSON() {
    XCTAssertNil(ChainSnapshot.fromSyncDeviceList(json: Data("{}".utf8), isSyncActive: true))
    XCTAssertNil(ChainSnapshot.fromSyncDeviceList(json: Data("nope".utf8), isSyncActive: true))
  }

  func testRoundTripsThroughAFile() throws {
    let url = temporaryDirectory().appendingPathComponent("transfer/devices.json")
    let snapshot = try XCTUnwrap(
      ChainSnapshot.fromSyncDeviceList(json: Data(deviceListJSON.utf8), isSyncActive: true))
    try snapshot.write(to: url)
    XCTAssertEqual(ChainSnapshot.read(from: url), snapshot)
  }

  func testTrustSourceNeedsKeyAndActiveSync() throws {
    let snapshot = try XCTUnwrap(
      ChainSnapshot.fromSyncDeviceList(json: Data(deviceListJSON.utf8), isSyncActive: true))
    let key = Bytes(repeating: 3, count: 32)
    let source = SnapshotTrustSource(snapshot: snapshot, rootKey: key)
    XCTAssertTrue(source.isSyncActive)
    XCTAssertEqual(source.chainKeys, try ChainKeys(rootKey: key))
    XCTAssertEqual(source.chainDevices.count, 3)

    var changes = 0
    source.onChange = { changes += 1 }
    source.update(snapshot: snapshot, rootKey: key)
    XCTAssertEqual(changes, 0)

    var inactive = snapshot
    inactive.isSyncActive = false
    source.update(snapshot: inactive, rootKey: key)
    XCTAssertEqual(changes, 1)
    XCTAssertFalse(source.isSyncActive)
    XCTAssertNil(source.chainKeys)
    XCTAssertTrue(source.chainDevices.isEmpty)
    XCTAssertEqual(source.localDeviceGUID, "")

    source.update(snapshot: snapshot, rootKey: Bytes(repeating: 1, count: 5))
    XCTAssertFalse(source.isSyncActive)
  }
}

final class TrustStoreTests: XCTestCase {
  func testPersistsAndReloads() {
    let url = temporaryDirectory().appendingPathComponent("transfer/trust.json")
    let store = FileTrustStore(url: url)
    store.update("a") {
      $0.staticKey = [1, 2, 3]
      $0.name = "Mac"
      $0.policy = .alwaysAsk
    }
    let other = FileTrustStore(url: url)
    XCTAssertEqual(
      other.record(for: "a"),
      TrustedDeviceRecord(staticKey: [1, 2, 3], name: "Mac", policy: .alwaysAsk))

    other.update("b") { $0.removed = true }
    XCTAssertEqual(store.record(for: "b")?.removed, true)

    store.remove(["a", "missing"])
    XCTAssertNil(FileTrustStore(url: url).record(for: "a"))
  }

  func testInMemory() {
    let store = FileTrustStore(url: nil)
    store.update("a") { $0.lastSeen = 5 }
    XCTAssertEqual(store.records.count, 1)
  }
}

final class InboxStoreTests: XCTestCase {
  private func entry(_ text: String) -> InboxEntry {
    InboxEntry(
      receivedTime: 1, deviceGUID: "g", deviceName: "Mac", kind: .text, text: text)
  }

  func testPagesNewestFirst() {
    let store = FileInboxStore(url: nil)
    for index in 0..<5 {
      XCTAssertEqual(store.add(entry("\(index)"))?.id, Int64(index + 1))
    }
    let first = store.page(offset: 0, count: 2)
    XCTAssertEqual(first.entries.map(\.text), ["4", "3"])
    XCTAssertTrue(first.hasMore)
    let last = store.page(offset: 4, count: 2)
    XCTAssertEqual(last.entries.map(\.text), ["0"])
    XCTAssertFalse(last.hasMore)
    XCTAssertEqual(store.page(offset: 10, count: 2).entries, [])
    XCTAssertEqual(store.entry(id: 3)?.text, "2")
  }

  func testPersistsAndClears() {
    let url = temporaryDirectory().appendingPathComponent("BraveTransfer/inbox.json")
    let store = FileInboxStore(url: url)
    store.add(entry("a"))
    store.add(entry("b"))
    XCTAssertEqual(FileInboxStore(url: url).page(offset: 0, count: 10).entries.count, 2)
    XCTAssertTrue(store.clear())
    let reloaded = FileInboxStore(url: url)
    XCTAssertTrue(reloaded.page(offset: 0, count: 10).entries.isEmpty)
    // Ids keep increasing after a clear.
    XCTAssertEqual(reloaded.add(entry("c"))?.id, 3)
  }

  func testMissingFile() throws {
    let directory = temporaryDirectory()
    let path = directory.appendingPathComponent("x.bin").path
    var entry = entry("")
    entry.filePath = path
    XCTAssertTrue(entry.isFileMissing)
    FileManager.default.createFile(atPath: path, contents: Data([1]))
    XCTAssertFalse(entry.isFileMissing)
    entry.filePath = nil
    XCTAssertFalse(entry.isFileMissing)
  }
}

final class SettingsStoreTests: XCTestCase {
  func testDefaultsAndRoundTrip() throws {
    let suite = "BraveTransferEngineTests.\(UUID().uuidString)"
    let defaults = try XCTUnwrap(UserDefaults(suiteName: suite))
    defer { defaults.removePersistentDomain(forName: suite) }
    let store = UserDefaultsSettingsStore(defaults: defaults)
    XCTAssertEqual(store.settings, TransferSettings())
    XCTAssertFalse(store.settings.autoAcceptTrusted)
    XCTAssertTrue(store.settings.autoTrustSyncChain)

    var settings = TransferSettings()
    settings.receiveMode = .off
    settings.autoAcceptTrusted = true
    settings.autoTrustSyncChain = false
    settings.notifyOnReceive = false
    store.settings = settings
    XCTAssertEqual(UserDefaultsSettingsStore(defaults: defaults).settings, settings)
    XCTAssertEqual(defaults.integer(forKey: TransferDefaultsKey.receiveMode), 0)
  }
}
