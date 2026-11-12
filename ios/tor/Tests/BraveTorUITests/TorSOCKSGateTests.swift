// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import Network
import XCTest
@testable import BraveTorUI

@MainActor
final class TorSOCKSGateTests: XCTestCase {
  func testGateWaitsForTorAndReusesEndpointAfterSuspension() async throws {
    let upstream = try NWListener(using: .tcp, on: .any)
    let ready = expectation(description: "Loopback upstream ready")
    upstream.stateUpdateHandler = { state in if case .ready = state { ready.fulfill() } }
    var accepted = 0
    let received = expectation(description: "First request relayed")
    let resumed = expectation(description: "Request relayed after reconnect")
    var forwardingAllowed = false
    var connections: [NWConnection] = []
    upstream.newConnectionHandler = { connection in
      MainActor.assumeIsolated {
        XCTAssertTrue(forwardingAllowed, "The gate forwarded traffic while suspended")
        accepted += 1
        let iteration = accepted
        connections.append(connection)
        connection.start(queue: .main)
        connection.receive(minimumIncompleteLength: 1, maximumLength: 32) { data, _, _, _ in
          XCTAssertEqual(data, Data("probe".utf8))
          (iteration == 1 ? received : resumed).fulfill()
        }
      }
    }
    upstream.start(queue: .main)
    defer { upstream.cancel(); connections.forEach { $0.cancel() } }
    await fulfillment(of: [ready], timeout: 5)
    let upstreamEndpoint = try TorEndpoint(listener: "127.0.0.1:\(try XCTUnwrap(upstream.port).rawValue)")
    let gate = TorSOCKSGate()
    let endpoint = try await gate.start()
    defer { gate.close() }
    let first = NWConnection(host: .ipv4(.loopback), port: .init(rawValue: endpoint.port)!, using: .tcp)
    first.start(queue: .main)
    first.send(content: Data("probe".utf8), completion: .contentProcessed { _ in })
    let premature = expectation(description: "No forwarding before Tor is ready")
    premature.isInverted = true
    await fulfillment(of: [premature], timeout: 0.2)
    XCTAssertEqual(accepted, 0)
    forwardingAllowed = true
    gate.forward(to: upstreamEndpoint)
    await fulfillment(of: [received], timeout: 5)
    forwardingAllowed = false
    gate.forward(to: nil)
    first.cancel()
    let second = NWConnection(host: .ipv4(.loopback), port: .init(rawValue: endpoint.port)!, using: .tcp)
    defer { second.cancel() }
    second.start(queue: .main)
    second.send(content: Data("probe".utf8), completion: .contentProcessed { _ in })
    let suspended = expectation(description: "No forwarding while suspended")
    suspended.isInverted = true
    await fulfillment(of: [suspended], timeout: 0.2)
    XCTAssertEqual(accepted, 1)
    forwardingAllowed = true
    gate.forward(to: upstreamEndpoint)
    await fulfillment(of: [resumed], timeout: 5)
    XCTAssertEqual(accepted, 2)
  }
}
#endif
