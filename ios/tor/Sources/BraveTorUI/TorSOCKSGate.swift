// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import Foundation
import Network

/// A stable loopback endpoint. It forwards opaque SOCKS bytes only to a ready
/// embedded Tor listener, never to a destination host or a system resolver.
@MainActor
final class TorSOCKSGate {
  private var listener: NWListener?
  private var endpoint: TorEndpoint?
  private var flows: [UUID: Flow] = [:]

  func start() async throws -> TorEndpoint {
    let parameters = NWParameters.tcp
    parameters.requiredLocalEndpoint = .hostPort(host: .ipv4(.loopback), port: .any)
    let listener = try NWListener(using: parameters)
    self.listener = listener
    listener.newConnectionHandler = { [weak self] connection in
      MainActor.assumeIsolated { self?.accept(connection) }
    }
    return try await withCheckedThrowingContinuation { continuation in
      var completed = false
      listener.stateUpdateHandler = { [weak self, weak listener] state in
        MainActor.assumeIsolated {
          guard !completed else { return }
          switch state {
          case .ready:
            completed = true
            guard let port = listener?.port else {
              continuation.resume(throwing: TorError.unavailable)
              return
            }
            continuation.resume(returning: try! TorEndpoint(listener: "\"127.0.0.1:\(port.rawValue)\""))
          case .failed(let error):
            completed = true
            self?.close()
            continuation.resume(throwing: error)
          case .cancelled:
            completed = true
            continuation.resume(throwing: CancellationError())
          default: break
          }
        }
      }
      listener.start(queue: .main)
    }
  }

  func forward(to endpoint: TorEndpoint?) {
    self.endpoint = endpoint
    if endpoint == nil {
      // In-flight sockets cannot survive a route change. Close them without
      // deleting the page, its DOM, form state, cookies, or back/forward list.
      for flow in Array(flows.values) where flow.isForwarding { flow.close() }
    } else if let endpoint {
      for flow in Array(flows.values) where !flow.isForwarding { flow.start(to: endpoint) }
    }
  }

  func close() {
    endpoint = nil
    for flow in Array(flows.values) { flow.close() }
    listener?.cancel()
    listener = nil
  }

  private func accept(_ connection: NWConnection) {
    guard flows.count < 128 else { connection.cancel(); return }
    let id = UUID()
    let flow = Flow(client: connection) { [weak self] in self?.flows.removeValue(forKey: id) }
    flows[id] = flow
    connection.stateUpdateHandler = { [weak flow] state in
      MainActor.assumeIsolated {
        switch state {
        case .failed, .cancelled: flow?.close()
        default: break
        }
      }
    }
    connection.start(queue: .main)
    if let endpoint { flow.start(to: endpoint) }
  }

  @MainActor
  private final class Flow {
    private let client: NWConnection
    private var upstream: NWConnection?
    private let onClose: () -> Void
    private var closed = false
    var isForwarding: Bool { upstream != nil }

    init(client: NWConnection, onClose: @escaping () -> Void) {
      self.client = client
      self.onClose = onClose
    }

    func start(to endpoint: TorEndpoint) {
      guard !closed, upstream == nil else { return }
      let upstream = NWConnection(
        host: .ipv4(.loopback), port: .init(rawValue: endpoint.port)!, using: .tcp
      )
      self.upstream = upstream
      upstream.stateUpdateHandler = { [weak self] state in
        MainActor.assumeIsolated {
          guard let self, !self.closed else { return }
          switch state {
          case .ready:
            self.pump(from: self.client, to: upstream)
            self.pump(from: upstream, to: self.client)
          case .failed, .cancelled: self.close()
          default: break
          }
        }
      }
      upstream.start(queue: .main)
    }

    private func pump(from source: NWConnection, to destination: NWConnection) {
      source.receive(minimumIncompleteLength: 1, maximumLength: 65_536) { [weak self] data, _, end, error in
        MainActor.assumeIsolated {
          guard let self, !self.closed else { return }
          guard error == nil, let data, !data.isEmpty else { self.close(); return }
          destination.send(content: data, completion: .contentProcessed { [weak self] error in
            MainActor.assumeIsolated {
              guard let self, !self.closed else { return }
              if error != nil || end { self.close() } else { self.pump(from: source, to: destination) }
            }
          })
        }
      }
    }

    func close() {
      guard !closed else { return }
      closed = true
      client.stateUpdateHandler = nil
      upstream?.stateUpdateHandler = nil
      client.cancel()
      upstream?.cancel()
      onClose()
    }
  }
}
#endif
