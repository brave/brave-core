// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransfer
import BraveTransferEngine
import Foundation
import os

/// Drives the "Brave Transfer" share extension's UI: loads the shared items,
/// runs `ShareSendFlow` on the engine queue and mirrors its state here, and
/// writes the outbox when the items go to the app.
@MainActor
public final class TransferShareModel: ObservableObject {
  public enum Problem: Equatable {
    /// None of the shared items can be sent.
    case nothingToSend
    /// Copying the items to the outbox failed.
    case handoffFailed
  }

  @Published public private(set) var snapshot = ShareSendFlow.Snapshot(
    phase: .preparing,
    devices: [],
    transfer: nil,
    isReadyToSend: false,
    lastSendResult: nil
  )
  @Published public private(set) var problem: Problem?
  @Published public private(set) var isHandingOff = false

  private let engine: TransferEngine
  private let flow: ShareSendFlow
  private let appScheme: String
  private let workDirectory: URL
  private let openURL: (URL) -> Void
  private let finish: () -> Void
  private var items: [SharedItem] = []
  private var didFinish = false

  nonisolated private static let log = Logger(subsystem: "com.brave.transfer", category: "share")

  /// - Parameters:
  ///   - appScheme: the app's URL scheme (`BRAVE_URL_SCHEME`), for the
  ///     hand-off URL.
  ///   - openURL: opens the app; the extension does this through the
  ///     responder chain.
  ///   - finish: completes the extension request.
  public init(
    engine: TransferEngine,
    appScheme: String,
    workDirectory: URL,
    openURL: @escaping (URL) -> Void,
    finish: @escaping () -> Void
  ) {
    self.engine = engine
    self.appScheme = appScheme
    self.workDirectory = workDirectory
    self.openURL = openURL
    self.finish = finish
    flow = engine.queue.sync {
      ShareSendFlow(service: engine.service, scheduler: engine.scheduler)
    }
    engine.queue.sync { [flow] in
      flow.onChange = { [weak self] snapshot in
        DispatchQueue.main.async {
          MainActor.assumeIsolated {
            self?.update(snapshot)
          }
        }
      }
    }
  }

  public func start(providers: [NSItemProvider]) {
    Task {
      let items = await SharedItemLoader.load(providers, into: workDirectory)
      guard !items.isEmpty else {
        problem = .nothingToSend
        return
      }
      self.items = items
      let total = items.totalSize
      let localNetwork = engine.localNetwork.lastKnownStatus
      let flow = self.flow
      engine.perform { _ in flow.begin(totalBytes: total, localNetwork: localNetwork) }
      guard flow.policy.reasonBeforeSending(totalBytes: total) == nil else { return }
      let engine = self.engine
      DispatchQueue.global(qos: .userInitiated).async {
        let outgoing = Self.outgoingItems(items)
        engine.perform { _ in flow.setItems(outgoing) }
      }
    }
  }

  nonisolated private static func outgoingItems(_ items: [SharedItem]) -> [OutgoingItem] {
    items.compactMap { item in
      do {
        return try item.outgoingItem()
      } catch {
        log.error("Could not read a shared file: \(error.localizedDescription)")
        return nil
      }
    }
  }

  public func send(to deviceID: String) {
    let flow = self.flow
    engine.perform { _ in flow.send(to: deviceID) }
  }

  public func continueInApp() {
    let flow = self.flow
    engine.perform { _ in flow.continueInApp() }
  }

  /// "Cancel" or "Done".
  public func close() {
    let flow = self.flow
    engine.perform { _ in flow.cancel() }
    guard case .sending = snapshot.phase else {
      cleanUp()
      complete()
      return
    }
    // Give the Cancel frame a moment to reach the computer; the extension
    // may be killed as soon as the request completes.
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
      MainActor.assumeIsolated {
        self.cleanUp()
        self.complete()
      }
    }
  }

  /// Copies the items into the app group's outbox and opens the app at
  /// `brave://transfer/send?outbox=<id>`.
  public func openApp() {
    guard !isHandingOff, !items.isEmpty else { return }
    isHandingOff = true
    let items = self.items
    let outbox = engine.appGroup.outbox
    let scheme = appScheme
    DispatchQueue.global(qos: .userInitiated).async {
      let url = Self.writeOutbox(items, to: outbox, scheme: scheme)
      DispatchQueue.main.async {
        MainActor.assumeIsolated {
          self.handOffFinished(url)
        }
      }
    }
  }

  nonisolated private static func writeOutbox(
    _ items: [SharedItem],
    to outbox: Outbox,
    scheme: String
  ) -> URL? {
    outbox.purge()
    let writer: OutboxWriter
    do {
      writer = try outbox.create()
    } catch {
      log.error("Could not create an outbox entry: \(error.localizedDescription)")
      return nil
    }
    do {
      for item in items {
        switch item {
        case .text(let text):
          writer.addText(text, isLink: false)
        case .link(let link):
          writer.addText(link, isLink: true)
        case .file(let url, let name, let mimeType, _):
          try writer.addFile(at: url, name: name, mimeType: mimeType, move: true)
        }
      }
      try writer.commit()
    } catch {
      log.error("Could not write the outbox: \(error.localizedDescription)")
      writer.discard()
      return nil
    }
    return Outbox.handoffURL(scheme: scheme, id: writer.id)
  }

  private func handOffFinished(_ url: URL?) {
    isHandingOff = false
    guard let url else {
      problem = .handoffFailed
      return
    }
    items = []
    openURL(url)
    cleanUp()
    complete()
  }

  private func update(_ snapshot: ShareSendFlow.Snapshot) {
    self.snapshot = snapshot
    if snapshot.phase == .handoff(.userRequested) {
      openApp()
    }
  }

  private func cleanUp() {
    try? FileManager.default.removeItem(at: workDirectory)
  }

  private func complete() {
    guard !didFinish else { return }
    didFinish = true
    finish()
  }
}
