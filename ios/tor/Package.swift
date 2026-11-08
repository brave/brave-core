// swift-tools-version: 5.9
// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import PackageDescription

let package = Package(
  name: "BraveTor",
  defaultLocalization: "en",
  platforms: [.iOS("18.0"), .macOS(.v13)],
  products: [
    .library(name: "TorWebKit", targets: ["TorWebKit"]),
    .library(name: "BraveTor", targets: ["BraveTor"]),
    .library(name: "BraveTorUI", targets: ["BraveTorUI"]),
  ],
  dependencies: [
    .package(url: "https://github.com/tladesignz/IPtProxy.git", exact: "5.7.0"),
  ],
  targets: [
    .binaryTarget(
      name: "tor",
      url: "https://github.com/iCepa/Tor.framework/releases/download/v409.14.1/tor.xcframework.zip",
      checksum: "823d98fcafd8eba089f31b32989ad70e15a0e8a1b5709bc0d5941fb09ef1b625"
    ),
    .target(
      name: "EmbeddedTor",
      dependencies: ["tor"],
      linkerSettings: [.linkedLibrary("z")]
    ),
    .target(
      name: "BraveTor",
      dependencies: ["EmbeddedTor", .product(name: "IPtProxy", package: "IPtProxy")],
      // IPtProxy's Go runtime references Darwin's res_9_* resolver symbols.
      linkerSettings: [.linkedLibrary("resolv")]
    ),
    .target(name: "TorWebKit"),
    .target(
      name: "BraveTorUI",
      dependencies: ["BraveTor", .target(name: "TorWebKit", condition: .when(platforms: [.iOS]))],
      resources: [.process("Resources")]
    ),
    .testTarget(name: "BraveTorTests", dependencies: ["BraveTor"]),
    .testTarget(name: "BraveTorUITests", dependencies: ["BraveTorUI", "BraveTor", "TorWebKit"]),
  ],
  swiftLanguageVersions: [.v5]
)
