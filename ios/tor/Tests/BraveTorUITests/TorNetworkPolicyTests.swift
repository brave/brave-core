// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Network
import XCTest
@testable import BraveTorUI

final class TorNetworkPolicyTests: XCTestCase {
  private let wifi = TorNetworkPath.Interface(index: 4, name: "en0")
  private let cellular = TorNetworkPath.Interface(index: 5, name: "pdp_ip0")
  private let vpn = TorNetworkPath.Interface(index: 8, name: "utun4")
  private let gateway: NWEndpoint = .hostPort(host: "192.0.2.1", port: 0)
  private let otherGateway: NWEndpoint = .hostPort(host: "192.0.2.2", port: 0)

  private func path(
    satisfied: Bool, interfaces: [TorNetworkPath.Interface], gateways: Set<NWEndpoint>
  ) -> TorNetworkPath {
    TorNetworkPath(isSatisfied: satisfied, interfaces: interfaces, gateways: gateways)
  }

  func testConnectWaitsForFirstSatisfiedPath() {
    var policy = TorNetworkPolicy()
    XCTAssertFalse(policy.canConnect)
    XCTAssertTrue(policy.update(path(satisfied: false, interfaces: [], gateways: [])))
    XCTAssertFalse(policy.canConnect)
    _ = policy.update(path(satisfied: true, interfaces: [wifi], gateways: [gateway]))
    XCTAssertTrue(policy.canConnect)
  }

  func testRepeatedCallbacksOnSameRouteDoNotRequestDisconnect() {
    var policy = TorNetworkPolicy()
    let route = path(satisfied: true, interfaces: [wifi], gateways: [gateway])
    XCTAssertFalse(policy.update(route))
    for _ in 0..<100 {
      XCTAssertFalse(policy.update(route))
      XCTAssertTrue(policy.canConnect)
    }
  }

  func testGatewayOrderingDoesNotDisconnect() {
    var policy = TorNetworkPolicy()
    let first = path(
      satisfied: true, interfaces: [wifi, vpn], gateways: Set([gateway, otherGateway])
    )
    let reordered = path(
      satisfied: true, interfaces: [wifi, vpn], gateways: Set([otherGateway, gateway])
    )
    XCTAssertFalse(policy.update(first))
    XCTAssertFalse(policy.update(reordered))
  }

  func testPreferredInterfaceChangeRequiresDisconnect() {
    var policy = TorNetworkPolicy()
    let wifiPreferred = path(
      satisfied: true, interfaces: [wifi, cellular], gateways: [gateway]
    )
    let cellularPreferred = path(
      satisfied: true, interfaces: [cellular, wifi], gateways: [gateway]
    )
    XCTAssertFalse(policy.update(wifiPreferred))
    XCTAssertTrue(policy.update(cellularPreferred))
    XCTAssertFalse(policy.update(cellularPreferred))
  }

  func testRouteTransitionsRequireDisconnect() {
    var policy = TorNetworkPolicy()
    _ = policy.update(path(satisfied: true, interfaces: [wifi], gateways: [gateway]))
    XCTAssertTrue(policy.update(path(satisfied: true, interfaces: [cellular], gateways: [])))
    XCTAssertTrue(policy.update(path(satisfied: true, interfaces: [vpn], gateways: [gateway])))
    XCTAssertTrue(policy.update(path(satisfied: true, interfaces: [wifi], gateways: [gateway])))
    // A new interface identity or gateway still invalidates a same-type route.
    let replacedWifi = TorNetworkPath.Interface(index: 9, name: "en0")
    XCTAssertTrue(
      policy.update(path(satisfied: true, interfaces: [replacedWifi], gateways: [gateway]))
    )
    XCTAssertTrue(
      policy.update(path(satisfied: true, interfaces: [replacedWifi], gateways: [otherGateway]))
    )
  }

  func testReachabilityLossRequiresDisconnect() {
    var policy = TorNetworkPolicy()
    _ = policy.update(path(satisfied: true, interfaces: [wifi], gateways: [gateway]))
    XCTAssertTrue(policy.update(path(satisfied: false, interfaces: [wifi], gateways: [gateway])))
    XCTAssertFalse(policy.canConnect)
    let restored = path(satisfied: true, interfaces: [wifi], gateways: [gateway])
    XCTAssertTrue(policy.update(restored))
    XCTAssertTrue(policy.canConnect)
    XCTAssertFalse(policy.update(restored))
  }

  func testRetryUsesLatestRouteBaseline() {
    var policy = TorNetworkPolicy()
    _ = policy.update(path(satisfied: true, interfaces: [wifi], gateways: [gateway]))
    let changed = path(satisfied: true, interfaces: [cellular], gateways: [])
    XCTAssertTrue(policy.update(changed))
    // The user explicitly retries after the route-change shutdown.
    XCTAssertTrue(policy.canConnect)
    XCTAssertFalse(policy.update(changed))
  }
}
