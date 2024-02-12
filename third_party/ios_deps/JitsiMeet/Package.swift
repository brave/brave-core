// swift-tools-version: 5.6
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "JitsiMeet",
  platforms: [.iOS(.v15), .macOS(.v11)],
  products: [
    .library(name: "JitsiMeet", targets: ["JitsiMeet", "JitsiMeetSDK"]),
  ],
  dependencies: [
    .package(url: "https://github.com/jitsi/webrtc", revision: "83690c8ecfd2d745eb0a41c77de6c4e740c18b80")
  ],
  targets: [
    .target(name: "JitsiMeet", dependencies: [.product(name: "WebRTC", package: "webrtc")]),
    .binaryTarget(
      name: "JitsiMeetSDK",
      path: "JitsiMeetSDK.xcframework"
    ),
  ]
)
