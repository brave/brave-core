// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CryptoKit
import SnapKit
import WebKit
import XCTest

@testable import Brave

final class ScriptExecutionTests: XCTestCase {
  struct FarblingTestDTO: Decodable {
    let voiceNames: [String]
    let pluginNames: [String]
    let hardwareConcurrency: Int
  }

  struct RequestBlockingTestDTO: Decodable {
    let blockedFetch: Bool
    let blockedXHR: Bool
  }

  struct CosmeticFilteringTestDTO: Decodable {
    let hiddenIds: [String]
    let unhiddenIds: [String]
    let removedElement: Bool
    let removedClass: Bool
    let removedAttribute: Bool
    let styledElement: Bool
  }

  @MainActor func testSiteStateListenerScript() async throws {
    // Given
    let viewController = MockScriptsViewController()

    // When
    let stream = viewController.attachScriptHandler(
      contentWorld: SiteStateListenerScriptHandler.scriptSandbox,
      name: SiteStateListenerScriptHandler.messageHandlerName
    )

    // Load the view and add the siteStateListener script
    viewController.loadViewIfNeeded()
    viewController.add(scripts: [.siteStateListener])

    // Load the sample htmls page and await the first page load result
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try! String(contentsOf: htmlURL, encoding: .utf8)
    try await viewController.loadHTMLStringAndWait(htmlString)

    // Then
    // Await the script handler and checks it's contents
    var foundMessage: SiteStateListenerScriptHandler.MessageDTO?
    for try await message in stream {
      do {
        let data = try JSONSerialization.data(withJSONObject: message.body)
        foundMessage = try JSONDecoder().decode(
          SiteStateListenerScriptHandler.MessageDTO.self,
          from: data
        )
      } catch {
        XCTFail(String(describing: error))
      }

      // We only care about the first script handler result
      break
    }

    // Ensure we got a result
    XCTAssertNotNil(foundMessage)
    XCTAssertEqual(foundMessage?.data.windowURL, "https://example.com/")
  }

  @MainActor func testFarblingProtectionScript() async throws {
    // Given
    let controlViewController = MockScriptsViewController()
    let farbledViewController = MockScriptsViewController()
    let etldP1 = "example.com"

    // When
    // Attach a farbling script test handler to both the view controller and control view controller
    let controlStream = controlViewController.attachScriptHandler(
      contentWorld: .page,
      name: "SendTestFarblingResult"
    )
    let stream = farbledViewController.attachScriptHandler(
      contentWorld: .page,
      name: "SendTestFarblingResult"
    )

    // Load the view
    controlViewController.loadViewIfNeeded()
    farbledViewController.loadViewIfNeeded()

    // Add the farblingProtection script
    // (ONLY to the farbled controller)
    let farblingScriptType = UserScriptType.farblingProtection(etld: etldP1)
    farbledViewController.add(scripts: [farblingScriptType])

    // Add the tests script to both view controllers
    let testURL = Bundle.module.url(forResource: "farbling-tests", withExtension: "js")!
    let source = try String(contentsOf: testURL)
    let testScript = WKUserScript(
      source: source,
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: .page
    )
    farbledViewController.add(userScript: testScript)
    controlViewController.add(userScript: testScript)

    // Load the sample htmls page and await the first page load result
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try! String(contentsOf: htmlURL, encoding: .utf8)
    async let load1: Void = controlViewController.loadHTMLStringAndWait(htmlString)
    async let load2: Void = farbledViewController.loadHTMLStringAndWait(htmlString)
    _ = try await (load1, load2)

    // Then
    // Await the results
    var farblingResult: FarblingTestDTO?
    for try await message in stream {
      do {
        let data = try JSONSerialization.data(withJSONObject: message.body)
        farblingResult = try JSONDecoder().decode(FarblingTestDTO.self, from: data)
      } catch {
        XCTFail(String(describing: error))
      }

      // We only care about the first script handler result
      break
    }
    var controlResult: FarblingTestDTO?
    for try await message in controlStream {
      do {
        let data = try JSONSerialization.data(withJSONObject: message.body)
        controlResult = try JSONDecoder().decode(FarblingTestDTO.self, from: data)
      } catch {
        XCTFail(String(describing: error))
      }

      // We only care about the first script handler result
      break
    }

    // Ensure we have some results
    XCTAssertNotNil(farblingResult)
    XCTAssertNotNil(controlResult)
    XCTAssertNotNil(farblingResult?.voiceNames)
    XCTAssertNotNil(controlResult?.voiceNames)
    XCTAssertNotNil(farblingResult?.pluginNames)
    XCTAssertNotNil(controlResult?.pluginNames)
    XCTAssertNotNil(farblingResult?.hardwareConcurrency)
    XCTAssertNotNil(controlResult?.hardwareConcurrency)

    // Ensure farbled and unfarbled results are not the same
    XCTAssertNotEqual(farblingResult?.voiceNames, controlResult?.voiceNames)
    XCTAssertNotEqual(farblingResult?.pluginNames, controlResult?.pluginNames)
  }

  @MainActor func testRequestBlockingScript() async throws {
    // Given
    let viewController = MockScriptsViewController()

    // When
    let blockingResultStream = viewController.attachScriptHandler(
      contentWorld: RequestBlockingContentScriptHandler.scriptSandbox,
      name: "SendTestRequestResult"
    )

    viewController.attachScriptHandler(
      contentWorld: RequestBlockingContentScriptHandler.scriptSandbox,
      name: RequestBlockingContentScriptHandler.messageHandlerName,
      messageHandler: MockMessageHandler(callback: { message in
        // Block the request
        return true
      })
    )

    // Load the view and add scripts
    viewController.loadViewIfNeeded()
    viewController.add(userScript: RequestBlockingContentScriptHandler.userScript!)

    let testURL = Bundle.module.url(forResource: "request-blocking-tests", withExtension: "js")!
    let source = try String(contentsOf: testURL)
    let testScript = WKUserScript(
      source: source,
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: RequestBlockingContentScriptHandler.scriptSandbox
    )
    viewController.add(userScript: testScript)

    // Load the sample htmls page and await the first page load result
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try String(contentsOf: htmlURL, encoding: .utf8)
    try await viewController.loadHTMLStringAndWait(htmlString)

    // Then
    // Await the script handler and checks it's contents
    var foundMessage: RequestBlockingTestDTO?
    for try await message in blockingResultStream {
      do {
        let data = try JSONSerialization.data(withJSONObject: message.body)
        foundMessage = try JSONDecoder().decode(RequestBlockingTestDTO.self, from: data)
      } catch {
        XCTFail(String(describing: error))
      }

      // We only care about the first script handler result
      break
    }

    // Ensure we got a result
    XCTAssertNotNil(foundMessage)
    XCTAssertEqual(foundMessage?.blockedFetch, true)
    XCTAssertEqual(foundMessage?.blockedXHR, true)
  }

  @MainActor func testCosmeticFilteringScript() async throws {
    // Given
    let viewController = MockScriptsViewController()
    let invalidSelectors = Set([
      "div.invalid-selector:has(span.update-components-actor__description:-abp-contains(/Anzeige|Sponsored|Promoted|Dipromosikan|Propagováno|Promoveret|Gesponsert|Promocionado|促銷內容|Post sponsorisé|프로모션|Post sponsorizzato|广告|プロモーション|Treść promowana|Patrocinado|Promovat|Продвигается|Marknadsfört|Nai-promote|ได้รับการโปรโมท|Öne çıkarılan içerik|Gepromoot|الترويج/))"
    ])
    let initialStandardSelectors = Set([".test-ads-primary-standard div"])
    let initialAggressiveSelectors = Set([".test-ads-primary-aggressive div"])
    let polledAggressiveIds = ["test-ad-aggressive"]
    let polledStandardIds = [
      "test-ad-1st-party", "test-ad-3rd-party", "test-ad-3rd-party-sig-text", "test-ad-simple",
    ]
    let nestedIds = [
      "test-ad-primary-standard-1st-party", "test-ad-primary-standard-3rd-party",
      "test-ad-primary-aggressive-1st-party", "test-ad-primary-aggressive-3rd-party",
    ]
    let setup = UserScriptType.SelectorsPollerSetup(
      hideFirstPartyContent: false,
      genericHide: false,
      firstSelectorsPollingDelayMs: nil,
      switchToSelectorsPollingThreshold: 1000,
      fetchNewClassIdRulesThrottlingMs: 100,
      aggressiveSelectors: initialAggressiveSelectors,
      standardSelectors: initialStandardSelectors.union(invalidSelectors)
    )

    AdblockEngine.setDomainResolver()
    let engine = try AdblockEngine(
      rules: [
        "brave.com###test-remove-element:remove()",
        "brave.com###test-remove-class:remove-class(test)",
        "brave.com###test-remove-attribute:remove-attr(test)",
        "brave.com###test-style-element:style(background-color: red !important)",
      ].joined(separator: "\n")
    )
    let proceduralFilters = try engine.cosmeticFilterModel(
      forFrameURL: URL(string: "https://brave.com")!
    )?.proceduralActions

    XCTAssertNotNil(proceduralFilters)

    // Attach fake message handlers for our CF script
    let selectorsMessageHandler = viewController.attachScriptHandler(
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      name: CosmeticFiltersScriptHandler.messageHandlerName,
      messageHandler: MockMessageHandler(callback: { message in
        do {
          let data = try JSONSerialization.data(withJSONObject: message.body)
          let dto = try JSONDecoder().decode(
            CosmeticFiltersScriptHandler.CosmeticFiltersDTO.self,
            from: data
          )

          for id in polledAggressiveIds {
            XCTAssertTrue(dto.data.ids.contains(id))
          }
          for id in polledStandardIds {
            XCTAssertTrue(dto.data.ids.contains(id))
          }
          for id in nestedIds {
            XCTAssertTrue(dto.data.ids.contains(id))
          }

          // Return selectors to hide
          return [
            "aggressiveSelectors": polledAggressiveIds.map({ "#\($0)" }),
            "standardSelectors": polledStandardIds.map({ "#\($0)" }),
          ]
        } catch {
          XCTFail(String(describing: error))
          return nil
        }
      })
    )

    viewController.attachScriptHandler(
      contentWorld: URLPartinessScriptHandler.scriptSandbox,
      name: URLPartinessScriptHandler.messageHandlerName,
      messageHandler: MockMessageHandler(callback: { message in
        do {
          let data = try JSONSerialization.data(withJSONObject: message.body)
          let dto = try JSONDecoder().decode(
            URLPartinessScriptHandler.PartinessDTO.self,
            from: data
          )

          XCTAssertEqual(dto.data.urls.count, 2)
          XCTAssertTrue(dto.data.urls.contains("http://1st_party.localhost"))
          XCTAssertTrue(dto.data.urls.contains("http://3rd_party.localhost"))

          // Return fake partiness information
          return [
            "http://1st_party.localhost": true,
            "http://3rd_party.localhost": false,
          ]
        } catch {
          XCTFail(String(describing: error))
          return nil
        }
      })
    )

    // When
    // Attach a result message handler
    let testResultMessageHandler = viewController.attachScriptHandler(
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      name: "SendCosmeticFiltersResult",
      messageHandler: MockMessageHandler(callback: { message in
        return nil
      })
    )
    viewController.attachScriptHandler(
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      name: "LoggingHandler",
      messageHandler: MockMessageHandler(callback: { message in
        print("Script Message: \(message.body)")
      })
    )

    // Load the view and add scripts
    viewController.loadViewIfNeeded()

    // Load the sample htmls page and await the first page load result
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try String(contentsOf: htmlURL, encoding: .utf8)
    try await viewController.loadHTMLStringAndWait(htmlString)

    // Execute the selectors poller script
    let script = try ScriptFactory.shared.makeScript(
      for: .selectorsPoller(setup, proceduralActions: Set(proceduralFilters ?? []))
    )
    try await viewController.webView.evaluateSafeJavaScriptThrowing(
      functionName: script.source,
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      asFunction: false
    )

    // Await the execution of the selectors message handler
    // (so we know we already hid our elements)
    for try await _ in selectorsMessageHandler {
      // We only care about the first script handler result
      break
    }

    // Now wait for the pump which takes a few seconds.
    // The pump unhides 1st party elements.
    try await Task.sleep(seconds: 5)

    // Then
    // Execute a script that will test the cosmetic filters page
    let testURL = Bundle.module.url(forResource: "cosmetic-filter-tests", withExtension: "js")!
    let source = try String(contentsOf: testURL)
    try await viewController.webView.evaluateSafeJavaScriptThrowing(
      functionName: source,
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      asFunction: false
    )

    // Await the results of the test script
    var resultsAfterPump: CosmeticFilteringTestDTO?
    for try await message in testResultMessageHandler {
      do {
        let data = try JSONSerialization.data(withJSONObject: message.body)
        resultsAfterPump = try JSONDecoder().decode(CosmeticFilteringTestDTO.self, from: data)
      } catch {
        XCTFail(String(describing: error))
      }

      // We don't listen for ever. Just the first test result
      break
    }

    XCTAssertNotNil(resultsAfterPump)
    XCTAssertEqual(resultsAfterPump?.unhiddenIds.contains("test-ad-1st-party"), true)
    XCTAssertEqual(
      resultsAfterPump?.unhiddenIds.contains("test-ad-primary-standard-1st-party"),
      true
    )
    XCTAssertEqual(
      resultsAfterPump?.unhiddenIds.contains("test-ad-primary-standard-3rd-party"),
      true
    )
    XCTAssertEqual(resultsAfterPump?.unhiddenIds.contains("test-ad-1st-party"), true)
    XCTAssertEqual(resultsAfterPump?.hiddenIds.contains("test-ad-aggressive"), true)
    // hidden because of 3rd party src
    XCTAssertEqual(resultsAfterPump?.hiddenIds.contains("test-ad-3rd-party"), true)
    // unhidden because it contains significant amount of text
    XCTAssertEqual(resultsAfterPump?.unhiddenIds.contains("test-ad-3rd-party-sig-text"), true)
    XCTAssertEqual(resultsAfterPump?.hiddenIds.contains("test-ad-simple"), true)
    XCTAssertEqual(
      resultsAfterPump?.hiddenIds.contains("test-ad-primary-aggressive-1st-party"),
      true
    )
    XCTAssertEqual(
      resultsAfterPump?.hiddenIds.contains("test-ad-primary-aggressive-3rd-party"),
      true
    )
    // Test for procedural filters
    XCTAssertTrue(resultsAfterPump?.removedElement ?? false)
    XCTAssertTrue(resultsAfterPump?.removedAttribute ?? false)
    XCTAssertTrue(resultsAfterPump?.removedClass ?? false)
    XCTAssertTrue(resultsAfterPump?.styledElement ?? false)
  }
}
