// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Foundation
import XCTest
@testable import BraveTor

final class TorRuntimeTests: XCTestCase {
  func testEmbeddedTorStartsWithNetworkingDisabled() async throws {
    let directory = FileManager.default.temporaryDirectory
      .appendingPathComponent("BraveTorTests-" + UUID().uuidString, isDirectory: true)
    let runtime = TorRuntime(dataDirectory: directory)
    // This test never calls connect(): no public Tor network is needed.
    try await runtime.prepare()
    let version = try await runtime.version()
    XCTAssertTrue(version.hasPrefix("0.4.9.14"), version)
    let disabled = try await runtime.networkIsDisabled()
    XCTAssertTrue(disabled)
    let snapshot = try await runtime.snapshot()
    XCTAssertFalse(snapshot.circuitEstablished)
    XCTAssertNil(snapshot.endpoint)
    let concurrent = TorRuntime(
      dataDirectory: directory.appendingPathComponent("concurrent", isDirectory: true)
    )
    do {
      try await concurrent.prepare()
      XCTFail("A second in-process Tor instance must be refused")
    } catch {
      XCTAssertEqual(error as? TorError, .unavailable)
    }
    let stillRunningVersion = try await runtime.version()
    XCTAssertEqual(stillRunningVersion, version)
    var invalid = TorBridgeConfiguration()
    invalid.useBridges = true
    invalid.source = .provide
    invalid.provided = ["192.0.2.1:443\nDisableNetwork 0"]
    await runtime.configureBridges(invalid)
    do {
      try await runtime.connect()
      XCTFail("Invalid bridge settings must keep Tor offline")
    } catch { }
    let remainsDisabled = try await runtime.networkIsDisabled()
    XCTAssertTrue(remainsDisabled)
    await runtime.suspend()
  }
}
