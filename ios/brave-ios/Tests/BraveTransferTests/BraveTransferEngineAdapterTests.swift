// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import Network
import XCTest
import dnssd

@testable import BraveTransfer

final class BraveTransferEngineAdapterTests: XCTestCase {
  private var defaults: UserDefaults!
  private var suiteName = ""
  private var directory: URL!

  override func setUp() {
    super.setUp()
    suiteName = "BraveTransferTests.\(UUID().uuidString)"
    defaults = UserDefaults(suiteName: suiteName)
    directory = FileManager.default.temporaryDirectory
      .appendingPathComponent(UUID().uuidString, isDirectory: true)
    try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
  }

  override func tearDown() {
    defaults.removePersistentDomain(forName: suiteName)
    try? FileManager.default.removeItem(at: directory)
    super.tearDown()
  }

  func testPolicyDeniedIsLocalNetworkDenial() {
    let denied = NWError.dns(DNSServiceErrorType(kDNSServiceErr_PolicyDenied))
    XCTAssertTrue(BonjourAdvertiser.isLocalNetworkDenied(denied))
    XCTAssertFalse(BonjourAdvertiser.isLocalNetworkDenied(.posix(.ECONNREFUSED)))
    XCTAssertFalse(
      BonjourAdvertiser.isLocalNetworkDenied(.dns(DNSServiceErrorType(kDNSServiceErr_NoSuchName)))
    )
  }

  func testLocalNetworkStatusFollowsAdvertiser() {
    let authorization = LocalNetworkAuthorization(defaults: defaults)
    XCTAssertEqual(authorization.lastKnownStatus, .notDetermined)

    authorization.record(AdvertiserState.starting)
    XCTAssertEqual(authorization.lastKnownStatus, .notDetermined)

    authorization.record(AdvertiserState.waiting(localNetworkDenied: true))
    XCTAssertEqual(authorization.lastKnownStatus, .denied)

    authorization.record(AdvertiserState.waiting(localNetworkDenied: false))
    XCTAssertEqual(authorization.lastKnownStatus, .denied)

    authorization.record(AdvertiserState.advertising)
    XCTAssertEqual(authorization.lastKnownStatus, .granted)

    XCTAssertEqual(LocalNetworkAuthorization(defaults: defaults).lastKnownStatus, .granted)
  }

  func testAppGroupLayout() {
    let group = TransferAppGroup(identifier: "group.test", containerURL: directory)
    XCTAssertEqual(group.directory.lastPathComponent, "BraveTransfer")
    XCTAssertTrue(FileManager.default.fileExists(atPath: group.directory.path))
    XCTAssertEqual(group.chainSnapshotURL.deletingLastPathComponent(), group.directory)
    XCTAssertEqual(group.trustStoreURL.deletingLastPathComponent(), group.directory)
  }

  func testNonMediaGoesToFiles() throws {
    let downloads = directory.appendingPathComponent("Downloads", isDirectory: true)
    let finalizer = ReceivedItemFinalizer(downloadsDirectory: { downloads })
    let temporary = directory.appendingPathComponent("item.part")
    try Data("hello".utf8).write(to: temporary)
    let file = IncomingFile(
      key: "t/1", name: "notes.txt", kind: .file, mimeType: "text/plain", size: 5, sha256: []
    )

    let saved = expectation(description: "saved")
    finalizer.finalize(file, temporaryURL: temporary) { result in
      XCTAssertEqual(result?.fileName, "notes.txt")
      XCTAssertEqual(
        result?.url?.deletingLastPathComponent().standardizedFileURL,
        downloads.standardizedFileURL
      )
      XCTAssertEqual(result?.savedToPhotos, false)
      XCTAssertEqual(result?.usedFallback, false)
      saved.fulfill()
    }
    wait(for: [saved], timeout: 5)
    XCTAssertTrue(
      FileManager.default.fileExists(atPath: downloads.appendingPathComponent("notes.txt").path)
    )
  }
}
