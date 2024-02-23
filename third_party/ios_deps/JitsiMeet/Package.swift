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
    .package(url: "https://github.com/jitsi/webrtc", revision: "f20affccefa9c096a64ca15066f4b786a2ada1f6") // 111.0.2
  ],
  targets: [
    .target(name: "JitsiMeet", dependencies: [.product(name: "WebRTC", package: "webrtc")]),
    .binaryTarget(
      name: "JitsiMeetSDK",
      path: "JitsiMeetSDK.xcframework"
    ),
  ]
)
