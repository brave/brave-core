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
    // We create an engine. `$to=` rules ask the domain resolver whether a
    // destination is only a public suffix. iOS builds adblock without the
    // embedded resolver, so this has to be installed first.
    AdblockEngine.setDomainResolver()
    let engine = try AdblockEngine(rules: String(contentsOf: sampleFilterListURL, encoding: .utf8))

    // Then
    // Serialize then deserialize the engine
    let serializedData = try engine.serialize()
    _ = try AdblockEngine(serializedData: serializedData)
  }
}
