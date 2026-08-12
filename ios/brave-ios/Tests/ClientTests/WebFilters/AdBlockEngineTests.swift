// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import XCTest

final class AdBlockEngineTests: XCTestCase {
  func testSerializeAndDeserialize() throws {
    // Given
    // Ad block data and engine
    let sampleFilterListURL = Bundle.module.url(
      forResource: "iodkpdagapdfkphljnddpjlldadblomo",
      withExtension: "txt"
    )!

    // When
    // We create an engine
    let engine = try AdblockEngine(rulesFileURL: sampleFilterListURL)

    // Then
    // Serialize then deserialize the engine
    let serializedEngineURL = FileManager.default.temporaryDirectory
      .appendingPathComponent("\(UUID().uuidString).dat")
    defer { try? FileManager.default.removeItem(at: serializedEngineURL) }
    try engine.serialize(to: serializedEngineURL)
    _ = try AdblockEngine(serializedFileURL: serializedEngineURL)
  }
}
