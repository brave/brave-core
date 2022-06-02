// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

// swiftlint:disable legacy_random

/// A class that helps in creating farbling data
class FarblingProtectionHelper {
  /// Represents `JSON` data that needs to be passed to `FarblingProtection.js`
  struct FarblingData: Encodable {
    /// Represents the `JSON` data that is needed to construct a fake WebKit `Plugin`
    struct FakePluginData: Encodable {
      let name: String
      let filename: String
      let description: String
      let mimeTypes: [FakeMimeTypeData]
    }

    /// Represents the `JSON` data that is needed to construct a fake WebKit `MimeType`
    struct FakeMimeTypeData: Encodable {
      let suffixes: String
      let type: String
      let description: String
    }

    /// A value between 0.99 and 1 to fudge audio data
    ///
    /// A value between 0.99 to 1 means the values in the destination will
    /// always be within the expected range of -1 and 1.
    /// This small decrease should not affect affect legitimite users of this api.
    /// But will affect fingerprinters by introducing a small random change.
    let fudgeFactor: Float
    /// A value representing a fake voice name that will be used to add a fake voice
    let fakeVoiceName: String
    /// Fake data that is to be used to construct fake plugins
    let fakePluginData: [FakePluginData]
    /// This value is used to get a random index between 0 and an unknown count
    ///
    /// It's important to have a value between 0 - 1 in order to be within the array bounds
    let randomVoiceIndexScale: Float
    /// This value is used to get a random value between 2 and window.navigator.hardwareConcurrency
    ///
    /// Must be a value between 0 and 1
    let randomHardwareIndexScale: Float
  }

  /// Variables representing the prefix of a randomly generated strings used as the plugin name
  private static let pluginNameFirstParts: [String?] = [
    "Chrome", "Chromium", "Brave", "Web", "Browser",
    "OpenSource", "Online", "JavaScript", "WebKit",
    "Web-Kit", "WK", nil
  ]

  /// Variables representing the middle of a randomly generated strings used as the plugin name
  private static let pluginNameSecondParts: [String?] = [
    "PDF", "Portable Document Format",
    "portable-document-format", "document", "doc",
    "PDF and PS", "com.adobe.pdf", nil
  ]

  /// Variables representing the suffix of a randomly generated strings used as the plugin name
  private static let pluginNameThirdParts: [String?] = [
    "Viewer", "Renderer", "Display", "Plugin",
    "plug-in", "plug in", "extension", nil
  ]

  /// A list of fake voice names to be used to generate a fake `SpeechSynthesizer` voice
  private static let fakeVoiceNames: [String] = [
    "Hubert", "Vernon", "Rudolph", "Clayton", "Irving",
    "Wilson", "Alva", "Harley", "Beauregard", "Cleveland",
    "Cecil", "Reuben", "Sylvester", "Jasper"
  ]

  static func makeFarblingParams(from randomConfiguration: RandomConfiguration) throws -> String {
    srand48(randomConfiguration.domainKeyHEX.hashValue)

    let farblingData = FarblingData(
      fudgeFactor: Float.seededRandom(in: 0.99...1),
      fakeVoiceName: fakeVoiceNames.seededRandom() ?? "",
      fakePluginData: FarblingProtectionHelper.makeFakePluginData(),
      randomVoiceIndexScale: Float(drand48()),
      randomHardwareIndexScale: Float(drand48())
    )

    let encoder = JSONEncoder()
    let data = try encoder.encode(farblingData)
    return String(data: data, encoding: .utf8)!
  }

  /// Generate fake plugin data to be injected into the farbling protection script
  private static func makeFakePluginData() -> [FarblingData.FakePluginData] {
    let pluginCount = UInt8.seededRandom(in: 1...3)

    // Generate 1 to 3 fake plugins
    return (0..<pluginCount).map { pluginIndex -> FarblingData.FakePluginData in
      let mimeTypesCount = UInt8.seededRandom(in: 1...3)

      // Generate 1 to 3 fake mime types
      let mimeTypes = (0..<mimeTypesCount).map { mimeTypeIndex -> FarblingData.FakeMimeTypeData in
        return FarblingData.FakeMimeTypeData(
          suffixes: "pdf",
          type: "application/pdf",
          description: randomPluginName()
        )
      }

      return FarblingData.FakePluginData(
        name: randomPluginName(),
        filename: "",
        description: randomPluginName(),
        mimeTypes: mimeTypes
      )
    }
  }

  /// Generate a random string using a prefix, middle and suffix where any of those may be empty.
  /// - Note: May result in an empty string.
  private static func randomPluginName() -> String {
    return [
      pluginNameFirstParts.seededRandom(),
      pluginNameSecondParts.seededRandom(),
      pluginNameThirdParts.seededRandom()
    ].compactMap({ $0 ?? nil }).joined(separator: " ")
  }
}

private extension FixedWidthInteger {
  /// Return a random value in the given range.
  ///
  /// Uses `drand48`, hence you need to seed it before using this function using `srand48`
  static func seededRandom(in range: ClosedRange<Self>) -> Self {
    let size = Double(range.upperBound - range.lowerBound)
    let offset = drand48() * size
    let value = Self(Double(range.lowerBound) + offset)
    return value
  }
}

private extension Float {
  /// Return a random float in the given range.
  ///
  /// Uses `drand48`, hence you need to seed it before using this function using `srand48`
  static func seededRandom(in range: ClosedRange<Self>) -> Self {
    let size = Double(range.upperBound - range.lowerBound)
    let offset = drand48() * size
    let value = Self(Double(range.lowerBound) + offset)
    return value
  }
}

private extension Array {
  /// Return a random value in the given range.
  ///
  /// Uses `drand48`, hence you need to seed it before using this function using `srand48`
  /// - Note: Will return a `nil` value only if the array is empty. You can safely force unswrap the result
  /// in cases where the array is not empty
  func seededRandom() -> Element? {
    guard !isEmpty else { return nil }
    let randomIndex = Int(UInt.seededRandom(in: 0...UInt(count - 1)))
    return self[randomIndex]
  }
}

// swiftlint:enable legacy_random
