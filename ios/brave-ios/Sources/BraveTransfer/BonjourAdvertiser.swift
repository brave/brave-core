// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import Foundation
import Network
import dnssd

/// Listens on a random TCP port and advertises it as `_brave-xfer._tcp`
/// under a random instance name, with the rotating discovery tag in the TXT
/// record (design plan §3.4). Desktops browse for the service and dial in.
/// Confined to `queue`, the engine queue.
public final class BonjourAdvertiser: TransferAdvertiser {
  public private(set) var state: AdvertiserState = .stopped {
    didSet {
      if state != oldValue {
        onStateChange?(state)
      }
    }
  }
  public var onStateChange: ((AdvertiserState) -> Void)?
  public var onConnection: ((ByteStream) -> Void)?

  private let queue: DispatchQueue
  private var listener: NWListener?
  private var instanceName = ""
  private var txtRecord: [String: String] = [:]

  public init(queue: DispatchQueue) {
    self.queue = queue
  }

  deinit {
    listener?.cancel()
  }

  static func parameters() -> NWParameters {
    let parameters = NWParameters.tcp
    parameters.includePeerToPeer = false
    parameters.acceptLocalOnly = true
    if let options = parameters.defaultProtocolStack.transportProtocol as? NWProtocolTCP.Options {
      options.enableKeepalive = true
      options.keepaliveIdle = 15
      options.noDelay = true
    }
    return parameters
  }

  public func start(txtRecord: [String: String]) {
    stop()
    self.txtRecord = txtRecord
    instanceName = DiscoveryConstants.randomInstanceName()
    let listener: NWListener
    do {
      listener = try NWListener(using: Self.parameters())
    } catch {
      state = .failed(String(describing: error))
      return
    }
    listener.service = service()
    listener.stateUpdateHandler = { [weak self, weak listener] newState in
      guard let self, let listener, listener === self.listener else { return }
      self.listenerStateChanged(newState)
    }
    listener.newConnectionHandler = { [weak self, weak listener] connection in
      guard let self, let listener, listener === self.listener else {
        connection.cancel()
        return
      }
      self.onConnection?(NWConnectionByteStream(connection: connection, queue: self.queue))
    }
    self.listener = listener
    state = .starting
    listener.start(queue: queue)
  }

  public func updateTXTRecord(_ txtRecord: [String: String]) {
    guard txtRecord != self.txtRecord else { return }
    self.txtRecord = txtRecord
    listener?.service = service()
  }

  public func stop() {
    guard let listener else { return }
    self.listener = nil
    listener.stateUpdateHandler = nil
    listener.newConnectionHandler = nil
    listener.cancel()
    state = .stopped
  }

  private func service() -> NWListener.Service {
    NWListener.Service(
      name: instanceName,
      type: DiscoveryConstants.serviceType,
      domain: "local.",
      txtRecord: NWTXTRecord(txtRecord)
    )
  }

  private func listenerStateChanged(_ newState: NWListener.State) {
    switch newState {
    case .setup:
      state = .starting
    case .ready:
      state = .advertising
    case .waiting(let error):
      state = .waiting(localNetworkDenied: Self.isLocalNetworkDenied(error))
    case .failed(let error):
      state =
        Self.isLocalNetworkDenied(error)
        ? .waiting(localNetworkDenied: true) : .failed(String(describing: error))
      listener?.cancel()
      listener = nil
    case .cancelled:
      break
    @unknown default:
      break
    }
  }

  /// How a Local Network denial surfaces in Network.framework.
  static func isLocalNetworkDenied(_ error: NWError) -> Bool {
    if case .dns(let code) = error {
      return code == DNSServiceErrorType(kDNSServiceErr_PolicyDenied)
    }
    return false
  }
}
