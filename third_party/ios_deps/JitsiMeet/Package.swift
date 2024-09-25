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
      url: "https://github.com/jitsi/webrtc", revision: "7c9f8e712da4d399f85f0a9bfec17f0a68ceb1fd")
  ],
  targets: [
    .target(name: "JitsiMeet", dependencies: [.product(name: "WebRTC", package: "webrtc")]),
    .binaryTarget(
      name: "JitsiMeetSDK",
      path: "JitsiMeetSDK.xcframework"
    ),
  ]
)
