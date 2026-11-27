// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import Combine
import Network
import UIKit
import WebKit

/// Immutable browsing context chosen when a Tor tab opens. Close through its
/// owning TorBrowsingSession when the tab closes, even if the tab is kept for undo.
@MainActor
public final class TorTabSession {
  fileprivate let id = UUID()
  fileprivate let ownerID: UUID
  fileprivate let keepTabsSeparate: Bool
  fileprivate var context: TorBrowsingContext?

  var isolationID: UUID? { context?.isolationID }
  public var websiteDataStore: WKWebsiteDataStore? { context?.websiteDataStore }

  public func sharesBrowsingData(with other: TorTabSession) -> Bool {
    guard ownerID == other.ownerID, let context else { return false }
    return context === other.context
  }

  fileprivate init(ownerID: UUID, keepTabsSeparate: Bool, context: TorBrowsingContext) {
    self.ownerID = ownerID
    self.keepTabsSeparate = keepTabsSeparate
    self.context = context
  }
}

@MainActor
fileprivate final class TorBrowsingContext {
  let isolationID = UUID()
  var websiteDataStore: WKWebsiteDataStore?
}

/// One process-wide runtime. Storage/proxy contexts outlive web-view recreation
/// but are retired when their last tab closes. Reconnects preserve tab content.
@MainActor
public final class TorBrowsingSession: NSObject {
  public static let shared = TorBrowsingSession(runtime: TorRuntime(), automaticallyConnect: true)
  public enum State: Equatable { case waiting, connecting(Int), connected, unavailable }
  public private(set) var state: State = .waiting
  private let stateSubject = CurrentValueSubject<State, Never>(.waiting)
  public var statePublisher: AnyPublisher<State, Never> { stateSubject.eraseToAnyPublisher() }
  public private(set) var bootstrapTag = ""
  public private(set) var bootstrapSummary = ""
  private var hasConnected = false
  public var isReconnecting: Bool {
    if case .connecting = state { return hasConnected }
    return false
  }
  private let runtime: TorRuntime
  private let ownerID = UUID()
  private var sharedContext: TorBrowsingContext?
  private var sharedTabSessions: Set<UUID> = []
  private let automaticallyConnect: Bool
  private var enabled = true
  private var bridgeConfiguration = TorBridgeConfiguration()
  private let gate: TorSOCKSGate
  private let monitor = NWPathMonitor()
  private var policy = TorNetworkPolicy()
  private var appActive = false
  private var clients: [UUID: (State) -> Void] = [:]
  private var sessionTask: Task<Void, Never>?
  private var configurationTask: Task<(TorEndpoint, WKContentRuleList), Error>?
  private var generation = 0
  private var transition: Task<Void, Never>?

  public convenience init(runtime: TorRuntime, automaticallyConnect: Bool) {
    self.init(runtime: runtime, automaticallyConnect: automaticallyConnect, gate: TorSOCKSGate())
  }

  init(runtime: TorRuntime, automaticallyConnect: Bool, gate: TorSOCKSGate) {
    self.runtime = runtime
    self.automaticallyConnect = automaticallyConnect
    self.gate = gate
    super.init()
    appActive = UIApplication.shared.applicationState == .active
    monitor.pathUpdateHandler = { [weak self] path in
      MainActor.assumeIsolated {
        guard let self else { return }
        let changed = self.policy.update(TorNetworkPath(path))
        if changed { self.reconnect() } else { self.connectIfNeeded() }
      }
    }
    monitor.start(queue: .main)
    NotificationCenter.default.addObserver(self, selector: #selector(suspend), name: UIApplication.willResignActiveNotification, object: nil)
    NotificationCenter.default.addObserver(self, selector: #selector(resume), name: UIApplication.didBecomeActiveNotification, object: nil)
  }

  deinit {
    monitor.cancel()
    sessionTask?.cancel()
    NotificationCenter.default.removeObserver(self)
  }

  public func attach(id: UUID, onChange: @escaping (State) -> Void) {
    clients[id] = onChange
    onChange(state)
    connectIfNeeded()
  }

  public func detach(id: UUID) {
    clients.removeValue(forKey: id)
    if clients.isEmpty {
      hasConnected = false
      pause()
    }
  }

  public func openTabSession(keepTabsSeparate: Bool) -> TorTabSession {
    let context: TorBrowsingContext
    if keepTabsSeparate {
      context = TorBrowsingContext()
    } else {
      context = sharedContext ?? TorBrowsingContext()
      sharedContext = context
    }
    let tabSession = TorTabSession(
      ownerID: ownerID, keepTabsSeparate: keepTabsSeparate, context: context
    )
    if !keepTabsSeparate { sharedTabSessions.insert(tabSession.id) }
    return tabSession
  }

  public func closeTabSession(_ tabSession: TorTabSession) {
    guard tabSession.ownerID == ownerID, tabSession.context != nil else { return }
    tabSession.context = nil
    if !tabSession.keepTabsSeparate {
      sharedTabSessions.remove(tabSession.id)
      // Retire synchronously: cancelled creations and undo references must not
      // make a later tab rejoin a session whose last tab has already closed.
      if sharedTabSessions.isEmpty { sharedContext = nil }
    }
  }

  public func configuration(for tabSession: TorTabSession) async throws -> WKWebViewConfiguration {
    guard tabSession.ownerID == ownerID, let context = tabSession.context else {
      throw TorError.unavailable
    }
    if configurationTask == nil {
      configurationTask = Task { [gate] in
        let endpoint = try await gate.start()
        return (endpoint, try await TorWebPolicy.compileRules())
      }
    }
    guard let configurationTask else { throw TorError.unavailable }
    let (endpoint, rules) = try await configurationTask.value
    try Task.checkCancellation()
    guard tabSession.context === context else { throw TorError.unavailable }
    if context.websiteDataStore == nil {
      context.websiteDataStore = TorWebPolicy.dataStore(
        endpoint: endpoint, isolationID: context.isolationID.uuidString
      )
    }
    guard let store = context.websiteDataStore else { throw TorError.unavailable }
    return try TorWebPolicy.configuration(
      websiteDataStore: store, rules: rules, javaScriptEnabled: true
    )
  }

  private func publish(_ state: State) {
    self.state = state
    if state == .connected { hasConnected = true }
    stateSubject.send(state)
    for callback in Array(clients.values) { callback(state) }
  }

  private func pause() {
    generation += 1
    sessionTask?.cancel()
    sessionTask = nil
    gate.forward(to: nil)
    publish(.waiting)
    let previous = transition
    transition = Task { [runtime] in
      await previous?.value
      await runtime.suspend()
    }
  }

  public func reconnect() {
    pause()
    connectIfNeeded()
  }

  public func applyConfiguration(enabled: Bool, bridges: TorBridgeConfiguration) {
    guard enabled != self.enabled || bridges != bridgeConfiguration else { return }
    self.enabled = enabled
    bridgeConfiguration = bridges
    pause()
    let previous = transition
    transition = Task { [runtime] in
      await previous?.value
      await runtime.configureBridges(bridges)
    }
    connectIfNeeded()
  }

  @objc private func suspend() { appActive = false; pause() }
  @objc private func resume() { appActive = true; connectIfNeeded() }

  private func connectIfNeeded() {
    guard enabled, automaticallyConnect, sessionTask == nil, !clients.isEmpty, appActive, policy.canConnect else { return }
    let expected = generation
    let previous = transition
    bootstrapTag = ""
    bootstrapSummary = ""
    publish(.connecting(0))
    sessionTask = Task { [weak self] in
      await previous?.value
      guard let self, expected == generation, !Task.isCancelled else { return }
      do {
        try await runtime.connect()
        while !Task.isCancelled, expected == generation {
          let snapshot = try await runtime.snapshot()
          guard expected == generation, !Task.isCancelled else { return }
          bootstrapTag = snapshot.bootstrapTag
          bootstrapSummary = snapshot.bootstrapSummary
          if snapshot.progress == 100, snapshot.circuitEstablished, let endpoint = snapshot.endpoint {
            gate.forward(to: endpoint)
            publish(.connected)
          } else {
            gate.forward(to: nil)
            publish(.connecting(snapshot.progress))
          }
          // Poll the private control socket to detect circuit/control failures.
          try await Task.sleep(nanoseconds: 500_000_000)
        }
      } catch {
        guard expected == generation, !Task.isCancelled else { return }
        gate.forward(to: nil)
        publish(.unavailable)
        await runtime.suspend()
        // Back off a failed runtime while retaining the loopback gate and pages.
        try? await Task.sleep(nanoseconds: 2_000_000_000)
        guard expected == generation, !Task.isCancelled else { return }
        sessionTask = nil
        connectIfNeeded()
      }
    }
  }
}
#endif
