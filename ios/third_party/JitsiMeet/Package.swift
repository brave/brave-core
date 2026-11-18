// swift-tools-version: 5.6
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "JitsiMeet",
  platforms: [.iOS(.v15), .macOS(.v11)],
  products: [
    .library(name: "JitsiMeet", targets: ["JitsiMeet", "JitsiMeetSDK"])
  ],
  dependencies: [
    .package(
      url: "https://github.com/jitsi/webrtc",
      from: "155.0.0"
    )
  ],
  targets: [
    .target(
      name: "JitsiMeet",
      dependencies: [.product(name: "WebRTC", package: "webrtc"), "hermes"]
    ),
    .binaryTarget(name: "hermes", path: "hermesvm.xcframework"),
    .binaryTarget(
      name: "JitsiMeetSDK",
      path: "JitsiMeetSDK.xcframework"
    ),
  ]
)
