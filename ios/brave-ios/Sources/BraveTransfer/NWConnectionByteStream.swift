// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import BraveTransferEngine
import Foundation
import Network

/// A `ByteStream` over an inbound `NWConnection`. Runs on the engine queue,
/// which is also the connection's queue, so every callback already arrives
/// on the engine's scheduler.
final class NWConnectionByteStream: ByteStream {
  static let maxReadLength = 64 * 1024

  weak var delegate: ByteStreamDelegate?
  private(set) var pendingWriteBytes = 0

  private let connection: NWConnection
  private let queue: DispatchQueue
  private var isClosed = false
  private var didReportClose = false

  init(connection: NWConnection, queue: DispatchQueue) {
    self.connection = connection
    self.queue = queue
  }

  func start() {
    connection.stateUpdateHandler = { [weak self] state in
      guard let self, !self.isClosed else { return }
      switch state {
      case .ready:
        self.receive()
      case .failed(let error):
        self.reportClose(error)
      case .waiting(let error):
        // An accepted connection that cannot make progress will not recover.
        self.reportClose(error)
      default:
        break
      }
    }
    connection.start(queue: queue)
  }

  func write(_ data: Bytes) {
    guard !isClosed, !data.isEmpty else { return }
    pendingWriteBytes += data.count
    connection.send(
      content: Data(data),
      completion: .contentProcessed { [weak self] error in
        guard let self, !self.isClosed else { return }
        self.pendingWriteBytes -= data.count
        if let error {
          self.reportClose(error)
          return
        }
        self.delegate?.byteStreamDidDrain(self)
      }
    )
  }

  func close() {
    guard !isClosed else { return }
    isClosed = true
    delegate = nil
    connection.stateUpdateHandler = nil
    connection.cancel()
  }

  private func receive() {
    connection.receive(minimumIncompleteLength: 1, maximumLength: Self.maxReadLength) {
      [weak self] content, _, isComplete, error in
      guard let self, !self.isClosed else { return }
      if let content, !content.isEmpty {
        self.delegate?.byteStream(self, didRead: Bytes(content))
        if self.isClosed { return }
      }
      if let error {
        self.reportClose(error)
      } else if isComplete {
        self.reportClose(nil)
      } else {
        self.receive()
      }
    }
  }

  private func reportClose(_ error: Error?) {
    guard !didReportClose else { return }
    didReportClose = true
    delegate?.byteStream(self, didCloseWithError: error)
  }
}
