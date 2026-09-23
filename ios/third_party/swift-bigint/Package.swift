// swift-tools-version: 5.10
import PackageDescription

let package = Package(
  name: "BigNumber",
  products: [
    .library(name: "BigNumber", targets: ["BigNumber"])
  ],
  dependencies: [],
  targets: [
    .target(name: "BigNumber", path: "src")
  ]
)
