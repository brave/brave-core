// swift-tools-version: 5.6
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "Static",
  products: [
    .library(name: "Static", targets: ["Static"]),
  ],
  dependencies: [],
  targets: [
    .target(name: "Static", dependencies: [], path: "Static", exclude: ["Info.plist", "Static.h", "Tests"]),
    .testTarget(name: "StaticTests", dependencies: ["Static"], path: "Static/Tests", exclude: ["Info.plist"]),
  ]
)
