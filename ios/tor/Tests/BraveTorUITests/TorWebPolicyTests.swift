// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import WebKit
import XCTest
@testable import BraveTorUI

@MainActor
final class TorWebPolicyTests: XCTestCase {
  func testStorageAndCredentialBoundariesForBothModes() async throws {
    let session = TorBrowsingSession(runtime: TorRuntime(), automaticallyConnect: false)
    let isolatedA = session.openTabSession(keepTabsSeparate: true)
    let sharedA = session.openTabSession(keepTabsSeparate: false)
    let isolatedB = session.openTabSession(keepTabsSeparate: true)
    let sharedB = session.openTabSession(keepTabsSeparate: false)
    let tabs = [isolatedA, sharedA, isolatedB, sharedB]
    defer { tabs.forEach { session.closeTabSession($0) } }
    XCTAssertNotEqual(isolatedA.isolationID, isolatedB.isolationID)
    XCTAssertNotEqual(isolatedA.isolationID, sharedA.isolationID)
    XCTAssertNotEqual(isolatedB.isolationID, sharedA.isolationID)
    XCTAssertEqual(sharedA.isolationID, sharedB.isolationID)

    var stores: [WKWebsiteDataStore] = []
    for tab in tabs {
      let configuration = try await session.configuration(for: tab)
      let store = configuration.websiteDataStore
      stores.append(store)
      XCTAssertFalse(store.isPersistent)
      XCTAssertEqual(store.proxyConfigurations.count, 1)
      XCTAssertFalse(try XCTUnwrap(store.proxyConfigurations.first).allowFailover)
      XCTAssertTrue(configuration.defaultWebpagePreferences.isLockdownModeEnabled)
      XCTAssertTrue(configuration.preferences.isTextInteractionEnabled)
    }
    XCTAssertFalse(stores[0] === stores[2])
    XCTAssertTrue(stores[1] === stores[3])
    XCTAssertFalse(stores[0] === stores[1])
    XCTAssertFalse(stores[2] === stores[1])

    // Use a persistent cookie to verify that even a long expiry cannot turn
    // either Tor store into persistent storage. No network is contacted.
    let name = "tor_test_" + UUID().uuidString
    let isolatedCookie = try XCTUnwrap(HTTPCookie(properties: [
      .name: name, .value: "isolated", .domain: "brave-tor-storage.invalid",
      .path: "/", .expires: Date.distantFuture,
    ]))
    let sharedCookie = try XCTUnwrap(HTTPCookie(properties: [
      .name: name, .value: "shared", .domain: "brave-tor-storage.invalid",
      .path: "/", .expires: Date.distantFuture,
    ]))
    await stores[0].httpCookieStore.setCookie(isolatedCookie)
    await stores[1].httpCookieStore.setCookie(sharedCookie)
    let values = await cookieValues(named: name, in: stores)
    XCTAssertEqual(values, ["isolated", "shared", nil, "shared"])
    let ordinaryStores = [WKWebsiteDataStore.default(), WKWebsiteDataStore.nonPersistent()]
    let ordinaryValues = await cookieValues(named: name, in: ordinaryStores)
    XCTAssertEqual(ordinaryValues, [nil, nil])
    XCTAssertTrue(ordinaryStores.allSatisfy { $0.proxyConfigurations.isEmpty })
  }

  func testLastSharedTabCloseRetiresDataEvenWithIsolatedAndUndoReferencesAlive() async throws {
    let session = TorBrowsingSession(runtime: TorRuntime(), automaticallyConnect: false)
    let isolated = session.openTabSession(keepTabsSeparate: true)
    let first = session.openTabSession(keepTabsSeparate: false)
    let second = session.openTabSession(keepTabsSeparate: false)
    defer { [isolated, first, second].forEach { session.closeTabSession($0) } }
    let oldConfiguration = try await session.configuration(for: first)
    let oldID = first.isolationID
    let name = "tor_lifetime_" + UUID().uuidString
    let cookie = try XCTUnwrap(HTTPCookie(properties: [
      .name: name, .value: "old-session", .domain: "brave-tor-storage.invalid", .path: "/",
    ]))
    await oldConfiguration.websiteDataStore.httpCookieStore.setCookie(cookie)
    session.closeTabSession(first)
    let survivor = try await session.configuration(for: second)
    XCTAssertTrue(survivor.websiteDataStore === oldConfiguration.websiteDataStore)
    session.reconnect()
    let reconnected = try await session.configuration(for: second)
    XCTAssertTrue(reconnected.websiteDataStore === survivor.websiteDataStore)
    XCTAssertEqual(second.isolationID, oldID)
    session.closeTabSession(second)
    let fresh = session.openTabSession(keepTabsSeparate: false)
    defer { session.closeTabSession(fresh) }
    let freshConfiguration = try await session.configuration(for: fresh)
    session.closeTabSession(second) // Closing twice must not retire a later cohort.
    let peer = session.openTabSession(keepTabsSeparate: false)
    defer { session.closeTabSession(peer) }
    let peerConfiguration = try await session.configuration(for: peer)
    XCTAssertTrue(peerConfiguration.websiteDataStore === freshConfiguration.websiteDataStore)
    XCTAssertNotEqual(fresh.isolationID, oldID)
    XCTAssertFalse(freshConfiguration.websiteDataStore === oldConfiguration.websiteDataStore)
    let freshValues = await cookieValues(named: name, in: [freshConfiguration.websiteDataStore])
    XCTAssertEqual(freshValues, [nil])
    XCTAssertNil(first.websiteDataStore)
    XCTAssertNil(second.websiteDataStore)
    do {
      _ = try await session.configuration(for: first)
      XCTFail("A closed tab session must not acquire a configuration")
    } catch {}

    // A fresh owner models a new app session; retained old configurations must
    // not be discoverable by the new owner or accepted across owners.
    let nextSession = TorBrowsingSession(runtime: TorRuntime(), automaticallyConnect: false)
    let nextTab = nextSession.openTabSession(keepTabsSeparate: false)
    defer { nextSession.closeTabSession(nextTab) }
    let nextConfiguration = try await nextSession.configuration(for: nextTab)
    XCTAssertNotEqual(nextTab.isolationID, fresh.isolationID)
    let nextValues = await cookieValues(named: name, in: [nextConfiguration.websiteDataStore])
    XCTAssertEqual(nextValues, [nil])
    do {
      _ = try await nextSession.configuration(for: fresh)
      XCTFail("A Tor store must not cross session owners")
    } catch {}
  }

  func testQueuedConfigurationCannotReviveRetiredSharedSession() async throws {
    let session = TorBrowsingSession(runtime: TorRuntime(), automaticallyConnect: false)
    let old = session.openTabSession(keepTabsSeparate: false)
    let oldID = old.isolationID
    let creation = Task { try await session.configuration(for: old) }
    session.closeTabSession(old)
    let fresh = session.openTabSession(keepTabsSeparate: false)
    defer { session.closeTabSession(fresh) }
    do {
      _ = try await creation.value
      XCTFail("Closed session configuration must fail")
    } catch {}
    _ = try await session.configuration(for: fresh)
    XCTAssertNotEqual(fresh.isolationID, oldID)
    XCTAssertNil(old.websiteDataStore)
  }

  private func cookieValues(
    named name: String, in stores: [WKWebsiteDataStore]
  ) async -> [String?] {
    var values: [String?] = []
    for store in stores {
      let cookies = await store.httpCookieStore.allCookies()
      values.append(cookies.first { $0.name == name }?.value)
    }
    return values
  }

  func testProtectionCompilationAndSeparateEphemeralStores() async throws {
    let rules = try await TorWebPolicy.compileRules()
    let endpoint = try TorEndpoint(listener: "127.0.0.1:9050")
    let first = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "first", javaScriptEnabled: false
    )
    let second = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "second", javaScriptEnabled: false
    )
    XCTAssertFalse(first.websiteDataStore.isPersistent)
    XCTAssertFalse(first.websiteDataStore === second.websiteDataStore)
    XCTAssertEqual(first.websiteDataStore.proxyConfigurations.count, 1)
    XCTAssertFalse(try XCTUnwrap(first.websiteDataStore.proxyConfigurations.first).allowFailover)
    XCTAssertFalse(first.defaultWebpagePreferences.allowsContentJavaScript)
    XCTAssertTrue(first.defaultWebpagePreferences.isLockdownModeEnabled)
    XCTAssertFalse(first.allowsAirPlayForMediaPlayback)
    XCTAssertFalse(first.allowsPictureInPictureMediaPlayback)
    XCTAssertFalse(first.preferences.javaScriptCanOpenWindowsAutomatically)
    XCTAssertTrue(first.preferences.isTextInteractionEnabled)
  }

  func testReusedStoreMustBeEphemeralAndFailClosed() async throws {
    let rules = try await TorWebPolicy.compileRules()
    for store in [WKWebsiteDataStore.default(), WKWebsiteDataStore.nonPersistent()] {
      XCTAssertThrowsError(try TorWebPolicy.configuration(
        websiteDataStore: store, rules: rules, javaScriptEnabled: true
      ))
    }
    let endpoint = try TorEndpoint(listener: "127.0.0.1:9050")
    let store = TorWebPolicy.dataStore(endpoint: endpoint, isolationID: "invalid-failover")
    var proxy = try XCTUnwrap(store.proxyConfigurations.first)
    proxy.allowFailover = true
    store.proxyConfigurations = [proxy]
    XCTAssertThrowsError(try TorWebPolicy.configuration(
      websiteDataStore: store, rules: rules, javaScriptEnabled: true
    ))
  }

  func testJavaScriptChoicePreservesNativeProtectionsAndNavigationPreferences() async throws {
    let rules = try await TorWebPolicy.compileRules()
    let endpoint = try TorEndpoint(listener: "127.0.0.1:9050")
    let configuration = try TorWebPolicy.configuration(
      endpoint: endpoint, rules: rules, isolationID: "script-test", javaScriptEnabled: true
    )
    XCTAssertTrue(configuration.defaultWebpagePreferences.allowsContentJavaScript)
    XCTAssertTrue(configuration.preferences.isTextInteractionEnabled)
    XCTAssertTrue(configuration.defaultWebpagePreferences.isLockdownModeEnabled)
    XCTAssertFalse(configuration.websiteDataStore.isPersistent)
    XCTAssertEqual(configuration.websiteDataStore.proxyConfigurations.count, 1)
    XCTAssertFalse(
      try XCTUnwrap(configuration.websiteDataStore.proxyConfigurations.first).allowFailover
    )
    XCTAssertFalse(configuration.preferences.javaScriptCanOpenWindowsAutomatically)
    XCTAssertFalse(configuration.allowsAirPlayForMediaPlayback)
    XCTAssertFalse(configuration.allowsPictureInPictureMediaPlayback)
    let navigation = WKWebpagePreferences()
    TorWebPolicy.applyPagePreferences(navigation, javaScriptEnabled: true)
    XCTAssertTrue(navigation.allowsContentJavaScript)
    TorWebPolicy.applyPagePreferences(navigation, javaScriptEnabled: false)
    XCTAssertFalse(navigation.allowsContentJavaScript)
    XCTAssertTrue(navigation.isLockdownModeEnabled)
  }
}
#endif
