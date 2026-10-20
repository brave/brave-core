// swift-tools-version:5.9
// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import PackageDescription

// On Apple platforms the package uses CryptoKit and has no dependencies, so
// adding it to ios/brave-ios/Package.swift pulls in nothing new. swift-crypto
// (the same API) is only needed to build and test on Linux.
#if os(Linux)
let packageDependencies: [Package.Dependency] = [
  .package(url: "https://github.com/apple/swift-crypto.git", "3.0.0"..<"5.0.0")
]
let coreDependencies: [Target.Dependency] = [
  .product(name: "Crypto", package: "swift-crypto")
]
#else
let packageDependencies: [Package.Dependency] = []
let coreDependencies: [Target.Dependency] = []
#endif

let package = Package(
  name: "BraveTransferCore",
  platforms: [.iOS(.v17), .macOS(.v14)],
  products: [
    .library(name: "BraveTransferCore", targets: ["BraveTransferCore"]),
    .library(name: "BraveTransferEngine", targets: ["BraveTransferEngine"]),
    .executable(name: "BraveTransferPeer", targets: ["BraveTransferPeer"]),
  ],
  dependencies: packageDependencies,
  targets: [
    .target(name: "BraveTransferCore", dependencies: coreDependencies),
    // Connections, trust, transfers, inbox and outbox on top of the core,
    // with the platform pieces (sockets, Bonjour, Photos, keychain) behind
    // protocols. Foundation only, so it is tested here on Linux too.
    .target(name: "BraveTransferEngine", dependencies: ["BraveTransferCore"]),
    .executableTarget(name: "BraveTransferPeer", dependencies: ["BraveTransferCore"]),
    .testTarget(name: "BraveTransferCoreTests", dependencies: ["BraveTransferCore"]),
    .testTarget(
      name: "BraveTransferEngineTests", dependencies: ["BraveTransferEngine", "BraveTransferCore"]
    ),
  ]
)
