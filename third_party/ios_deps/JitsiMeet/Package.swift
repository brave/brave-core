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
      // webrtc 118.0.0
      url: "https://github.com/jitsi/webrtc", revision: "a52d059956976c2cba24b6b29ef1edfaf4c9cadf")
  ],
  targets: [
    .target(name: "JitsiMeet", dependencies: [.product(name: "WebRTC", package: "webrtc")]),
    .binaryTarget(
      name: "JitsiMeetSDK",
      path: "JitsiMeetSDK.xcframework"
    ),
  ]
)
