// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import Foundation
import Network
import WebKit
import XCTest
@testable import BraveTorUI
#if canImport(WebTabContracts)
@testable import WebTabContracts
#endif

@MainActor
final class TorSOCKSIntegrationTests: XCTestCase {
  #if canImport(WebTabContracts)
  func testDownloadsAndRedirectsRetainSOCKSCredentialsInBothStorageModes() async throws {
    for separate in [true, false] {
      try await exerciseDownload(separate: separate, slow: false, cancelBeforeStart: false)
    }
  }

  func testClosingGateFailsDownloadAndRemovesItsPartialFile() async throws {
    try await exerciseDownload(separate: true, slow: true, cancelBeforeStart: false)
  }

  func testCancellingDownloadBeforeDestinationDoesNotCreateAFile() async throws {
    try await exerciseDownload(separate: true, slow: false, cancelBeforeStart: true)
  }

  private func exerciseDownload(separate: Bool, slow: Bool, cancelBeforeStart: Bool) async throws {
    let gate = TorSOCKSGate()
    let session = TorBrowsingSession(runtime: TorRuntime(), automaticallyConnect: false, gate: gate)
    let tabSession = session.openTabSession(keepTabsSeparate: separate)
    let identity = try XCTUnwrap(tabSession.isolationID).uuidString
    let ready = expectation(description: "Download SOCKS server ready")
    let requests = expectation(description: "Download requests reach SOCKS with unresolved hostnames")
    requests.expectedFulfillmentCount = slow ? 1 : 2
    let server = try SOCKSProbeServer(serveScriptedPage: false, ready: { ready.fulfill() },
      destination: { hostname in
        XCTAssertTrue(["brave-tor-download.invalid", "brave-tor-download-file.invalid",
                       "brave-tor-download-slow.invalid"].contains(hostname ?? ""))
      }, observedRequest: { _, username, password in
        XCTAssertEqual(username, identity)
        XCTAssertEqual(password, identity)
        requests.fulfill()
      })
    let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    defer {
      gate.close(); server.stop(); session.closeTabSession(tabSession)
      try? FileManager.default.removeItem(at: directory)
    }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    gate.forward(to: try TorEndpoint(listener: "127.0.0.1:\(try XCTUnwrap(server.port))"))
    let configuration = try await session.configuration(for: tabSession)
    XCTAssertFalse(configuration.websiteDataStore.isPersistent)
    XCTAssertTrue(configuration.websiteDataStore.proxyConfigurations.allSatisfy { !$0.allowFailover })
    let view = WKWebView(frame: .zero, configuration: configuration)
    let destination = directory.appendingPathComponent("result.txt")
    let started = expectation(description: "WebKit offered a destination")
    let finished = expectation(description: "Transfer finished or failed")
    let probe = TorDownloadProbe(destination: destination, cancelBeforeStart: cancelBeforeStart,
      started: started, finished: finished)
    let host = slow ? "brave-tor-download-slow.invalid" : "brave-tor-download.invalid"
    let url = try XCTUnwrap(URL(string: "http://\(host)/file"))
    view.startDownload(using: URLRequest(url: url)) { probe.adopt($0, url: url) }
    await fulfillment(of: [started, requests], timeout: 20)
    if slow { gate.close() }
    await fulfillment(of: [finished], timeout: 20)
    if slow || cancelBeforeStart {
      XCTAssertNotNil(probe.error)
      XCTAssertFalse(FileManager.default.fileExists(atPath: destination.path))
    } else {
      XCTAssertNil(probe.error)
      XCTAssertEqual(try Data(contentsOf: destination), Data("tor-download-payload".utf8))
      XCTAssertEqual(probe.transfer?.filename, "report.txt")
      XCTAssertEqual(probe.transfer?.mimeType, "application/octet-stream")
    }
  }
  #endif

  func testAuthenticationOnTheWireMatchesTabAndSharedSessionBoundaries() async throws {
    let gate = TorSOCKSGate()
    let session = TorBrowsingSession(
      runtime: TorRuntime(), automaticallyConnect: false, gate: gate
    )
    let tabs = [true, true, false, false].map { session.openTabSession(keepTabsSeparate: $0) }
    let hosts = ["tab-one.invalid", "tab-two.invalid", "shared-one.invalid", "shared-two.invalid"]
    var identities: [String: String] = [:]
    var requests: [String: XCTestExpectation] = [:]
    for (host, tab) in zip(hosts, tabs) {
      identities[host] = try XCTUnwrap(tab.isolationID).uuidString
      requests[host] = expectation(description: "Authenticated SOCKS request for \(host)")
    }
    XCTAssertNotEqual(identities[hosts[0]], identities[hosts[1]])
    XCTAssertNotEqual(identities[hosts[0]], identities[hosts[2]])
    XCTAssertEqual(identities[hosts[2]], identities[hosts[3]])
    let expectedIdentities = identities
    let expectedRequests = requests
    let ready = expectation(description: "Authentication probe ready")
    let server = try SOCKSProbeServer(
      serveScriptedPage: false, ready: { ready.fulfill() }, destination: { _ in },
      observedRequest: { hostname, username, password in
        guard let hostname, let expected = expectedIdentities[hostname] else {
          XCTFail("Unexpected SOCKS destination")
          return
        }
        XCTAssertEqual(username, expected, "SOCKS username must identify the frozen browsing context")
        XCTAssertEqual(password, expected, "SOCKS must not silently select unauthenticated mode")
        expectedRequests[hostname]?.fulfill()
      }
    )
    var views: [WKWebView] = []
    defer {
      views.forEach { $0.stopLoading() }
      tabs.forEach { session.closeTabSession($0) }
      gate.close()
      server.stop()
    }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    gate.forward(to: try TorEndpoint(listener: "127.0.0.1:\(try XCTUnwrap(server.port))"))
    for (host, tab) in zip(hosts, tabs) {
      let configuration = try await session.configuration(for: tab)
      let view = WKWebView(frame: .zero, configuration: configuration)
      views.append(view)
      view.load(URLRequest(url: try XCTUnwrap(URL(string: "https://\(host)/"))))
    }
    await fulfillment(of: Array(expectedRequests.values), timeout: 20)
  }

  func testCookiesLocalStorageAndIndexedDBFollowStorageMode() async throws {
    let ready = expectation(description: "Storage probe ready")
    let server = try SOCKSProbeServer(
      serveScriptedPage: false, serveStoragePage: true,
      ready: { ready.fulfill() }, destination: { hostname in
        XCTAssertEqual(hostname, "brave-tor-storage.invalid")
      }
    )
    let gate = TorSOCKSGate()
    let session = TorBrowsingSession(
      runtime: TorRuntime(), automaticallyConnect: false, gate: gate
    )
    defer { gate.close(); server.stop() }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    gate.forward(to: try TorEndpoint(listener: "127.0.0.1:\(try XCTUnwrap(server.port))"))
    for keepSeparate in [true, false] {
      let first = session.openTabSession(keepTabsSeparate: keepSeparate)
      let second = session.openTabSession(keepTabsSeparate: keepSeparate)
      defer { session.closeTabSession(first); session.closeTabSession(second) }
      let firstConfiguration = try await session.configuration(for: first)
      let secondConfiguration = try await session.configuration(for: second)
      let writer = WKWebView(frame: .zero, configuration: firstConfiguration)
      let reader = WKWebView(frame: .zero, configuration: secondConfiguration)
      defer { writer.stopLoading(); reader.stopLoading() }
      try await loadStoragePage(in: writer)
      try await loadStoragePage(in: reader)
      let database = "tor_storage_" + UUID().uuidString
      _ = try await writer.callAsyncJavaScript("""
        document.cookie = 'tor_storage=written;path=/;SameSite=Lax';
        localStorage.setItem('tor_storage', 'written');
        await new Promise((resolve, reject) => {
          const request = indexedDB.open(database, 1);
          request.onupgradeneeded = () => request.result.createObjectStore('values');
          request.onerror = () => reject(request.error);
          request.onblocked = () => reject(new Error('database blocked'));
          request.onsuccess = () => {
            const db = request.result;
            const transaction = db.transaction('values', 'readwrite');
            transaction.objectStore('values').put('written', 'marker');
            transaction.oncomplete = () => { db.close(); resolve(); };
            transaction.onabort = () => { db.close(); reject(transaction.error); };
          };
        });
        return true;
        """, arguments: ["database": database], in: nil, contentWorld: .page)
      let result = try await reader.callAsyncJavaScript("""
        const stored = await new Promise((resolve, reject) => {
          const request = indexedDB.open(database, 1);
          request.onupgradeneeded = () => request.result.createObjectStore('values');
          request.onerror = () => reject(request.error);
          request.onblocked = () => reject(new Error('database blocked'));
          request.onsuccess = () => {
            const db = request.result;
            const transaction = db.transaction('values', 'readonly');
            const read = transaction.objectStore('values').get('marker');
            transaction.oncomplete = () => { db.close(); resolve(read.result || ''); };
            transaction.onabort = () => { db.close(); reject(transaction.error); };
          };
        });
        const cookie = document.cookie.split(';').some(value => value.trim() === 'tor_storage=written');
        return [cookie ? 'written' : '', localStorage.getItem('tor_storage') || '', stored].join('|');
        """, arguments: ["database": database], in: nil, contentWorld: .page)
      XCTAssertEqual(result as? String, keepSeparate ? "||" : "written|written|written")
    }
  }

  private func loadStoragePage(in view: WKWebView) async throws {
    let finished = expectation(description: "Storage document loaded through SOCKS")
    let probe = PageLoadProbe { finished.fulfill() }
    view.navigationDelegate = probe
    view.load(URLRequest(url: try XCTUnwrap(URL(string: "http://brave-tor-storage.invalid/"))))
    await fulfillment(of: [finished], timeout: 15)
    withExtendedLifetime(probe) {}
    XCTAssertEqual(view.url?.host, "brave-tor-storage.invalid")
  }

  func testWebKitPassesDestinationHostnameToSOCKS() async throws {
    let ready = expectation(description: "Local SOCKS listener is ready")
    let request = expectation(description: "SOCKS receives the unresolved hostname")
    let server = try SOCKSProbeServer(
      serveScriptedPage: false,
      ready: { ready.fulfill() },
      destination: { hostname in
        XCTAssertEqual(hostname, "brave-tor-probe.invalid")
        request.fulfill()
      }
    )
    defer { server.stop() }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    let port = try XCTUnwrap(server.port)
    let endpoint = try TorEndpoint(listener: "127.0.0.1:\(port)")
    let rules = try await TorWebPolicy.compileRules()
    let configuration = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "test-isolation", javaScriptEnabled: false
    )
    let webView = WKWebView(frame: .zero, configuration: configuration)
    // .invalid is reserved. The local stub rejects the SOCKS connection:
    // this test never contacts the public Tor network or an origin server.
    webView.load(URLRequest(url: URL(string: "https://brave-tor-probe.invalid/")!))
    await fulfillment(of: [request], timeout: 15)
    webView.stopLoading()
  }

  func testWebKitPassesOnionHostnameToSOCKSWithoutLocalResolution() async throws {
    let hostname = String(repeating: "a", count: 56) + ".onion"
    let ready = expectation(description: "Local SOCKS listener is ready")
    let request = expectation(description: "Unresolved onion hostname reaches SOCKS")
    let server = try SOCKSProbeServer(
      serveScriptedPage: false,
      ready: { ready.fulfill() },
      destination: { destination in
        XCTAssertEqual(destination, hostname)
        request.fulfill()
      }
    )
    defer { server.stop() }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    let endpoint = try TorEndpoint(listener: "127.0.0.1:\(try XCTUnwrap(server.port))")
    let rules = try await TorWebPolicy.compileRules()
    let configuration = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "onion-test", javaScriptEnabled: false
    )
    let webView = WKWebView(frame: .zero, configuration: configuration)
    defer { webView.stopLoading() }
    let url = try XCTUnwrap(TorDestination.onionURL(from: hostname + "/"))
    webView.load(URLRequest(url: url))
    // The loopback stub rejects CONNECT; no onion service or public resolver is contacted.
    await fulfillment(of: [request], timeout: 15)
  }

  func testPageJavaScriptFetchAndRedirectUseUnresolvedSOCKSHostnames() async throws {
    let ready = expectation(description: "Local SOCKS listener is ready")
    let document = expectation(description: "Document hostname reaches SOCKS")
    let fetch = expectation(description: "JavaScript fetch hostname reaches SOCKS")
    let redirect = expectation(description: "Redirect hostname reaches SOCKS")
    let completed = expectation(description: "Page JavaScript reads the fetch response")
    let server = try SOCKSProbeServer(
      serveScriptedPage: true,
      ready: { ready.fulfill() },
      destination: { hostname in
        switch hostname {
        case "brave-tor-probe.invalid": document.fulfill()
        case "brave-tor-fetch.invalid": fetch.fulfill()
        case "brave-tor-redirect.invalid": redirect.fulfill()
        default: XCTFail("Unexpected or resolved SOCKS destination: \(hostname ?? "nil")")
        }
      }
    )
    defer { server.stop() }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    let port = try XCTUnwrap(server.port)
    let endpoint = try TorEndpoint(listener: "127.0.0.1:\(port)")
    let rules = try await TorWebPolicy.compileRules()
    let configuration = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "script-isolation", javaScriptEnabled: true
    )
    let probe = ScriptResultProbe { value in
      XCTAssertEqual(value, "tor-script-ok")
      completed.fulfill()
    }
    configuration.userContentController.add(probe, name: "scriptResult")
    defer { configuration.userContentController.removeScriptMessageHandler(forName: "scriptResult") }
    let webView = WKWebView(frame: .zero, configuration: configuration)
    defer { webView.stopLoading() }
    // HTTP is intentional: the loopback SOCKS stub is also the origin server.
    // Reserved .invalid hostnames never resolve or contact a public destination.
    webView.load(URLRequest(url: URL(string: "http://brave-tor-probe.invalid/")!))
    await fulfillment(of: [document, fetch, redirect, completed], timeout: 20)
    XCTAssertEqual(webView.title, "script-ran")
  }

  func testHLSPlaybackUsesTorSOCKSCredentialsForPlaylistAndSegment() async throws {
    try XCTSkipUnless(TorWebPolicy.supportsMediaPlayback, "Exclusive AVFoundation loading unavailable")
    let ready = expectation(description: "Media SOCKS server ready")
    let playlist = expectation(description: "Playlist hostname reaches SOCKS")
    let segment = expectation(description: "Segment hostname reaches SOCKS")
    let playing = expectation(description: "HLS playback advances")
    playlist.assertForOverFulfill = false
    segment.assertForOverFulfill = false

    let identity = "media-isolation"
    let server = try SOCKSProbeServer(
      serveScriptedPage: true, ready: { ready.fulfill() }, destination: { _ in },
      observedRequest: { hostname, username, password in
        XCTAssertEqual(username, identity)
        XCTAssertEqual(password, identity)
        switch hostname {
        case "brave-tor-media-page.invalid": break
        case "brave-tor-media-playlist.invalid": playlist.fulfill()
        case "brave-tor-media-segment.invalid": segment.fulfill()
        default: XCTFail("Unexpected media destination: \(hostname ?? "nil")")
        }
      }
    )
    let gate = TorSOCKSGate()
    defer { gate.close(); server.stop() }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    gate.forward(to: try TorEndpoint(listener: "127.0.0.1:\(try XCTUnwrap(server.port))"))
    let endpoint = try await gate.start()
    let rules = try await TorWebPolicy.compileRules()
    let configuration = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: identity, javaScriptEnabled: true
    )
    // Permit scripted playback in this test so it can run without a UI tap.
    configuration.mediaTypesRequiringUserActionForPlayback = []
    let probe = ScriptResultProbe { result in
      XCTAssertEqual(result, "playing")
      playing.fulfill()
    }
    configuration.userContentController.add(probe, name: "scriptResult")
    defer { configuration.userContentController.removeScriptMessageHandler(forName: "scriptResult") }
    let view = WKWebView(frame: CGRect(x: 0, y: 0, width: 320, height: 240), configuration: configuration)
    defer { view.stopLoading() }
    let url = try XCTUnwrap(URL(string: "http://brave-tor-media-page.invalid/"))
    view.load(URLRequest(url: url))
    await fulfillment(of: [playlist, segment, playing], timeout: 25)
  }

  func testDisabledPageJavaScriptDoesNotExecute() async throws {
    let ready = expectation(description: "Local SOCKS listener is ready")
    let document = expectation(description: "Disabled-script document reaches SOCKS")
    let server = try SOCKSProbeServer(
      serveScriptedPage: true,
      ready: { ready.fulfill() },
      destination: { hostname in
        XCTAssertEqual(hostname, "brave-tor-probe.invalid", "Disabled scripts must not fetch")
        if hostname == "brave-tor-probe.invalid" { document.fulfill() }
      }
    )
    defer { server.stop() }
    server.start()
    await fulfillment(of: [ready], timeout: 10)
    let port = try XCTUnwrap(server.port)
    let rules = try await TorWebPolicy.compileRules()
    let endpoint = try TorEndpoint(listener: "127.0.0.1:\(port)")
    let configuration = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "disabled-script", javaScriptEnabled: false
    )
    let finished = expectation(description: "Static page finished loading")
    let probe = PageLoadProbe { finished.fulfill() }
    let webView = WKWebView(frame: .zero, configuration: configuration)
    webView.navigationDelegate = probe
    defer { webView.stopLoading() }
    // The navigation callback and title update arrive independently from WebKit.
    let title = keyValueObservingExpectation(
      for: webView, keyPath: "title", expectedValue: "script-off"
    )
    // Use the same scripted HTTP response as the enabled test, through SOCKS.
    webView.load(URLRequest(url: URL(string: "http://brave-tor-probe.invalid/")!))
    await fulfillment(of: [document, finished, title], timeout: 15)
    withExtendedLifetime(probe) {
      XCTAssertEqual(webView.title, "script-off")
    }
  }
}

#if canImport(WebTabContracts)
@MainActor
private final class TorDownloadProbe {
  var transfer: TorDownload?
  var error: Error?
  let destination: URL
  let cancelBeforeStart: Bool
  let started: XCTestExpectation
  let finished: XCTestExpectation

  init(destination: URL, cancelBeforeStart: Bool, started: XCTestExpectation, finished: XCTestExpectation) {
    self.destination = destination
    self.cancelBeforeStart = cancelBeforeStart
    self.started = started
    self.finished = finished
  }

  func adopt(_ download: WKDownload, url: URL) {
    transfer = TorDownload(task: download, originalURL: url, ready: { [weak self] transfer in
      guard let self else { transfer.cancelTransfer(); return }
      if self.cancelBeforeStart { transfer.cancelTransfer() }
      else { transfer.startDownloadToLocalFileAtPath(self.destination.path) }
      self.started.fulfill()
    }, finished: { [weak self] _, error in
      self?.error = error
      self?.finished.fulfill()
    })
  }
}
#endif

@MainActor
private final class ScriptResultProbe: NSObject, WKScriptMessageHandler {
  private let result: (String?) -> Void

  init(result: @escaping (String?) -> Void) { self.result = result }

  func userContentController(
    _ userContentController: WKUserContentController, didReceive message: WKScriptMessage
  ) {
    result(message.body as? String)
  }
}

@MainActor
private final class PageLoadProbe: NSObject, WKNavigationDelegate {
  private let finished: () -> Void

  init(finished: @escaping () -> Void) { self.finished = finished }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    finished()
  }
}

/// NWConnection callbacks and connection storage are confined to queue.
private final class SOCKSProbeServer: @unchecked Sendable {
  private let listener: NWListener
  private let queue = DispatchQueue(label: "brave.tor.socks-test")
  private let ready: () -> Void
  private let destination: (String?) -> Void
  private let serveScriptedPage: Bool
  private let serveStoragePage: Bool
  private let observedRequest: ((String?, String?, String?) -> Void)?
  private var connections: [NWConnection] = []

  var port: UInt16? { listener.port?.rawValue }

  init(
    serveScriptedPage: Bool, serveStoragePage: Bool = false, ready: @escaping () -> Void,
    destination: @escaping (String?) -> Void,
    observedRequest: ((String?, String?, String?) -> Void)? = nil
  ) throws {
    let parameters = NWParameters.tcp
    parameters.requiredLocalEndpoint = .hostPort(host: .ipv4(.loopback), port: .any)
    listener = try NWListener(using: parameters)
    self.ready = ready
    self.destination = destination
    self.serveScriptedPage = serveScriptedPage
    self.serveStoragePage = serveStoragePage
    self.observedRequest = observedRequest
  }

  func start() {
    listener.stateUpdateHandler = { [weak self] state in
      if case .ready = state { self?.ready() }
    }
    listener.newConnectionHandler = { [weak self] connection in
      guard let self else { return }
      self.connections.append(connection)
      connection.start(queue: self.queue)
      self.greeting(connection)
    }
    listener.start(queue: queue)
  }

  func stop() {
    listener.cancel()
    queue.async { [weak self] in
      self?.connections.forEach { $0.cancel() }
      self?.connections.removeAll()
    }
  }

  private func read(
    _ connection: NWConnection, count: Int, completion: @escaping (Data) -> Void
  ) {
    connection.receive(minimumIncompleteLength: count, maximumLength: count) {
      data, _, _, error in
      guard error == nil, let data, data.count == count else {
        connection.cancel()
        return
      }
      completion(data)
    }
  }

  private func send(
    _ data: [UInt8], to connection: NWConnection, completion: @escaping () -> Void
  ) {
    connection.send(content: Data(data), completion: .contentProcessed { error in
      if error == nil { completion() } else { connection.cancel() }
    })
  }

  private func greeting(_ connection: NWConnection) {
    read(connection, count: 2) { [weak self] header in
      guard let self, header[0] == 5, header[1] > 0 else {
        connection.cancel()
        return
      }
      self.read(connection, count: Int(header[1])) { [weak self] methods in
        guard let self else { return }
        if methods.contains(2) {
          self.send([5, 2], to: connection) { [weak self] in
            self?.authenticate(connection)
          }
        } else if methods.contains(0) {
          self.send([5, 0], to: connection) { [weak self] in
            self?.connectRequest(connection)
          }
        } else {
          connection.cancel()
        }
      }
    }
  }

  private func authenticate(_ connection: NWConnection) {
    read(connection, count: 2) { [weak self] header in
      guard let self, header[0] == 1, header[1] > 0 else {
        connection.cancel()
        return
      }
      self.read(connection, count: Int(header[1]) + 1) { [weak self] userAndLength in
        guard let self, let length = userAndLength.last, length > 0 else {
          connection.cancel()
          return
        }
        let username = String(data: userAndLength.dropLast(), encoding: .utf8)
        self.read(connection, count: Int(length)) { [weak self] passwordData in
          let password = String(data: passwordData, encoding: .utf8)
          self?.send([1, 0], to: connection) { [weak self] in
            self?.connectRequest(connection, username: username, password: password)
          }
        }
      }
    }
  }

  private func connectRequest(
    _ connection: NWConnection, username: String? = nil, password: String? = nil
  ) {
    read(connection, count: 4) { [weak self] header in
      guard let self else { return }
      guard header[0] == 5, header[1] == 1, header[3] == 3 else {
        self.destination(nil)
        connection.cancel()
        return
      }
      self.read(connection, count: 1) { [weak self] length in
        guard let self, length[0] > 0 else {
          connection.cancel()
          return
        }
        self.read(connection, count: Int(length[0]) + 2) { [weak self] address in
          guard let self else { return }
          let hostname = String(data: address.dropLast(2), encoding: .utf8)
          self.observedRequest?(hostname, username, password)
          self.destination(hostname)
          if self.serveScriptedPage || self.serveStoragePage, let hostname {
            self.send([5, 0, 0, 1, 0, 0, 0, 0, 0, 0], to: connection) { [weak self] in
              self?.readHTTPRequest(connection, hostname: hostname, buffer: Data())
            }
          } else {
            self.send([5, 1, 0, 1, 0, 0, 0, 0, 0, 0], to: connection) {
              connection.cancel()
            }
          }
        }
      }
    }
  }

  private func readHTTPRequest(_ connection: NWConnection, hostname: String, buffer: Data) {
    connection.receive(minimumIncompleteLength: 1, maximumLength: 8192) {
      [weak self] data, _, complete, error in
      guard let self, error == nil, let data, !data.isEmpty else {
        connection.cancel()
        return
      }
      let received = buffer + data
      guard received.count <= 8192 else {
        connection.cancel()
        return
      }
      if received.range(of: Data("\r\n\r\n".utf8)) != nil {
        self.respondToHTTP(connection, hostname: hostname)
      } else if !complete {
        self.readHTTPRequest(connection, hostname: hostname, buffer: received)
      } else {
        connection.cancel()
      }
    }
  }

  private func respondToHTTP(_ connection: NWConnection, hostname: String) {
    let status: String
    let headers: String
    let body: String
    if hostname == "brave-tor-download-slow.invalid" {
      // Keep the response open until the gate closes to exercise partial-transfer failure.
      connection.send(content: Data(("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
        + "Content-Length: 200000\r\nContent-Disposition: attachment; filename=report.txt\r\n\r\npartial").utf8),
        completion: .contentProcessed { _ in })
      return
    }
    switch hostname {
    case "brave-tor-media-page.invalid":
      status = "200 OK"
      headers = "Content-Type: text/html; charset=utf-8\r\n"
      body = """
        <!doctype html><video muted playsinline></video><script>
        const video = document.querySelector('video');
        let reported = false;
        function report(value) {
          if (reported) return;
          reported = true;
          window.webkit.messageHandlers.scriptResult.postMessage(value);
        }
        video.addEventListener('timeupdate', () => {
          if (video.currentTime > 0 && video.videoWidth > 0) report('playing');
        });
        video.addEventListener('error', () => report('media error ' + video.error.code));
        video.src = 'http://brave-tor-media-playlist.invalid/playlist.m3u8';
        video.play().catch(error => report(String(error)));
        </script>
        """
    case "brave-tor-media-playlist.invalid":
      status = "200 OK"
      headers = "Content-Type: application/vnd.apple.mpegurl\r\n"
      body = """
        #EXTM3U
        #EXT-X-VERSION:3
        #EXT-X-TARGETDURATION:1
        #EXTINF:1.0,
        http://brave-tor-media-segment.invalid/clip.ts
        #EXT-X-ENDLIST

        """
    case "brave-tor-media-segment.invalid":
      sendHTTP(connection, status: "200 OK", headers: "Content-Type: video/mp2t\r\n",
               body: Self.mediaSegment)
      return
    case "brave-tor-download.invalid":
      status = "302 Found"
      headers = "Location: http://brave-tor-download-file.invalid/file\r\n"
      body = ""
    case "brave-tor-download-file.invalid":
      status = "200 OK"
      headers = "Content-Type: application/octet-stream\r\nContent-Disposition: attachment; filename=report.txt\r\n"
      body = "tor-download-payload"
    case "brave-tor-storage.invalid" where serveStoragePage:
      status = "200 OK"
      headers = "Content-Type: text/html; charset=utf-8\r\n"
      body = "<!doctype html><title>Storage probe</title><body>Tor storage probe</body>"
    case "brave-tor-probe.invalid":
      status = "200 OK"
      headers = "Content-Type: text/html; charset=utf-8\r\n"
      body = """
        <!doctype html><title>script-off</title><script>
        document.title = 'script-ran';
        fetch('http://brave-tor-fetch.invalid/start')
          .then(response => response.text())
          .then(body => window.webkit.messageHandlers.scriptResult.postMessage(body))
          .catch(error => window.webkit.messageHandlers.scriptResult.postMessage(String(error)));
        </script>
        """
    case "brave-tor-fetch.invalid":
      status = "302 Found"
      headers = "Location: http://brave-tor-redirect.invalid/result\r\n"
      body = ""
    case "brave-tor-redirect.invalid":
      status = "200 OK"
      headers = "Content-Type: text/plain\r\n"
      body = "tor-script-ok"
    default:
      connection.cancel()
      return
    }
    sendHTTP(connection, status: status, headers: headers, body: Data(body.utf8))
  }

  private func sendHTTP(_ connection: NWConnection, status: String, headers: String, body: Data) {
    let response = "HTTP/1.1 \(status)\r\n\(headers)"
      + "Access-Control-Allow-Origin: *\r\nCache-Control: no-store\r\n"
      + "Content-Length: \(body.count)\r\nConnection: close\r\n\r\n"
    connection.send(
      content: Data(response.utf8) + body, contentContext: .finalMessage, isComplete: true,
      completion: .contentProcessed { error in
        if error != nil { connection.cancel() }
      }
    )
  }

  // One second of generated black H.264 video and silent AAC audio in MPEG-TS.
  private static let mediaSegment = Data(base64Encoded: """
    R0AREABC8CUAAcEAAP8B/wAB/IAUSBIBBkZGbXBlZwlTZXJ2aWNlMDF3fEPK////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////9HQAAQAACwDQABwQAAAAHwACqxBLL/////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////0dQABAAArAXAAHBAADhAPAAG+EA8AAP
    4QHwAC9EuZv/////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////R0EAMGhQ
    AAALQH4A////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////wAAAeAAAICABSEA
    AS0BAAAAAQnwAAAAAWdCwAraEJsBEAAAAwAQAAADAIDxImoAAAABaM4PyAAAAWWIhDomKAAJAsnJ
    yddddddddddddeBHQQEwjUD/////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////wAAAcAAJICABSEAAQAB
    //FgQAOf/N4CAExhdmM2MC4zMS4xMDIAAjBADkdBATGeQP//////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////8AAAHAABOAgAUhAAEtAf/xYEABf/wBGCAHR0EBMp5A////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////wAAAcAAE4CABSEAAVoB//FgQAF/
    /AEYIAdHQQEznkD/////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////AAAB
    wAATgIAFIQABhwH/8WBAAX/8ARggB0dBADGZEAAANzJ+AP//////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////AAAB4AAAgIAFIQAB3MkAAAABCfAAAAABQZogFKCMR0EBNJ5A////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////wAAAcAAE4CABSEAAbQB//FgQAF//AEYIAdH
    QQE1nkD/////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////AAABwAATgIAF
    IQAB4QH/8WBAAX/8ARggB0dBATaeQP//////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //////////8AAAHAABOAgAUhAAMOAf/xYEABf/wBGCAHR0EBN55A////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////wAAAcAAE4CABSEAAzsB//FgQAF//AEYIAdHQBERAELw
    JQABwQAA/wH/AAH8gBRIEgEGRkZtcGVnCVNlcnZpY2UwMXd8Q8r/////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////0dBADKZEAAAYyR+AP//////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////AAAB
    4AAAgIAFIQADjJEAAAABCfAAAAABQZpAFaCMR0EBOJ5A////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////wAAAcAAE4CABSEAA2gB//FgQAF//AEYIAdHQQE5nkD/////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////AAABwAATgIAFIQADlQH/8WBAAX/8
    ARggB0dBATqeQP//////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////8AAAHA
    ABOAgAUhAAPCAf/xYEABf/wBGCAHR0EBO55A////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////wAAAcAAE4CABSEAA+8B//FgQAF//AEYIAdHQQAzmRAAAI8WfgD/////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////wAAAeAAAICABSEABTxZAAAAAQnwAAAAAUGaYBWgjEdB
    ATyeQP//////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////8AAAHAABOAgAUh
    AAUcAf/xYEABf/wBGCAHR0EBPZ5A////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /////////wAAAcAAE4CABSEABUkB//FgQAF//AEYIAdHQQE+nkD/////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////AAABwAATgIAFIQAFdgH/8WBAAX/8ARggB0dBAT+eQP//
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////8AAAHAABOAgAUhAAWjAf/x
    YEABf/wBGCAHR0EBMJ5A////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    /wAAAcAAE4CABSEABdAB//FgQAF//AEYIAc=
    """, options: .ignoreUnknownCharacters)!
}
#endif
