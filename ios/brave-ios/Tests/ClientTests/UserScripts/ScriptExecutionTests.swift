// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CryptoKit
import Preferences
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
    let upwardInt: Bool
    let upwardSelector: Bool
    let localFrameElement: Bool
    let hasTextDisplayIsNone: Bool
    let hasDisplayIsNone: Bool
    let delayedHasTextHidden: Bool
    let delayedChildHasTextHidden: [Bool]
    let altMutationStrategyHidden: Bool
  }

  override class func setUp() {
    BraveCoreMain.initializeResourceBundleForTesting()
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
    if BraveCore.FeatureList.kBraveIOSEnableFarblingPlugins.enabled {
      XCTAssertNotEqual(farblingResult?.pluginNames, controlResult?.pluginNames)
    }
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
      forMainFrameOnly: false,
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
    let viewController = MockScriptsViewController()
    let siteURL = URL(string: "https://brave.com")!
    let invalidSelectors = Set([
      "div.invalid-selector:has(span.update-components-actor__description:-abp-contains(/Anzeige|Sponsored|Promoted|Dipromosikan|Propagováno|Promoveret|Gesponsert|Promocionado|促銷內容|Post sponsorisé|프로모션|Post sponsorizzato|广告|プロモーション|Treść promowana|Patrocinado|Promovat|Продвигается|Marknadsfört|Nai-promote|ได้รับการโปรโมท|Öne çıkarılan içerik|Gepromoot|الترويج/))"
    ])
    let initialStandardSelectors = Set([".test-ads-primary-standard div"])
    let initialAggressiveSelectors = Set([
      ".test-ads-primary-aggressive div", "test-local-frame-ad",
    ])
    let polledAggressiveIds = ["test-ad-aggressive"]
    let polledStandardIds = [
      "test-ad-1st-party", "test-ad-3rd-party", "test-ad-3rd-party-sig-text", "test-ad-simple",
    ]
    let nestedIds = [
      "test-ad-primary-standard-1st-party", "test-ad-primary-standard-3rd-party",
      "test-ad-primary-aggressive-1st-party", "test-ad-primary-aggressive-3rd-party",
    ]
    AdblockEngine.setDomainResolver()
    let engine = try AdblockEngine(
      rules: [
        "brave.com###test-remove-element:remove()",
        "brave.com###test-remove-class:remove-class(test)",
        "brave.com###test-remove-attribute:remove-attr(test)",
        "brave.com###test-style-element:style(background-color: red !important)",
        "brave.com###test-upward-int-target:upward(2)",
        "brave.com###test-upward-selector-target:upward(#test-upward-selector)",
        "brave.com###test-has-text:has-text(hide me)",
        "brave.com###test-has:has(a.banner-link)",
        "brave.com###test-delayed-has-text:has-text(hide me)",
        "brave.com##.procedural-filter-child-node-class:has-text(View in App)",
        "brave.com###test-alt-mutation-observation-strategy:has-text(hide me)",
      ].joined(separator: "\n")
    )
    let cosmeticFilterModel = try engine.cosmeticFilterModel(forFrameURL: siteURL)!
    // We only care about our AdblockEngine for this test
    let models: [AdBlockGroupsManager.CosmeticFilterModelTuple] = [(false, cosmeticFilterModel)]
    let setup = UserScriptType.ContentCosmeticSetup.makeSetup(
      from: models,
      isAggressive: false,
      cachedStandardSelectors: initialStandardSelectors.union(invalidSelectors),
      cachedAggressiveSelectors: initialAggressiveSelectors
    )
    let proceduralFilters = cosmeticFilterModel.proceduralActions
    XCTAssertNotNil(proceduralFilters)

    // Add mock SiteStateListenerScriptHandler and inject the SiteStateListener script so we get a message from each frame with the `WKFrameInfo`.
    let siteStateListenerMockMessageHandler = viewController.attachScriptHandler(
      contentWorld: SiteStateListenerScriptHandler.scriptSandbox,
      name: SiteStateListenerScriptHandler.messageHandlerName
    )
    // Attach mock message handlers for our selectors messages & url partiness messages
    let selectorsMessageHandler = viewController.attachScriptHandler(
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      name: CosmeticFiltersScriptHandler.messageHandlerName,
      messageHandler: MockMessageHandler(callback: { message in
        // Return selectors to hide
        return [
          "aggressiveSelectors": polledAggressiveIds.map({ "#\($0)" }),
          "standardSelectors": polledStandardIds.map({ "#\($0)" }),
        ]
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
    // Add test result message handler to receive result from the `cosmetic-filter-tests` testing script
    let testResultMessageHandler = viewController.attachScriptHandler(
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      name: "SendCosmeticFiltersResult",
      messageHandler: MockMessageHandler(callback: { message in
        return nil
      })
    )
    // Debug/Logging script for `sendDebugMessage` message in `cosmetic-filter-tests`
    viewController.attachScriptHandler(
      contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
      name: "LoggingHandler",
      messageHandler: MockMessageHandler(callback: { message in
        print("Script Message: \(message.body)")
        return nil
      })
    )
    // Load the web view and it's message handlers
    viewController.loadViewIfNeeded()
    viewController.add(scripts: [.siteStateListener])

    // Load test html in the web view
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try String(contentsOf: htmlURL, encoding: .utf8)
    try await viewController.loadHTMLStringAndWait(htmlString, baseURL: siteURL)

    // Inject content cosmetic script into each frame received from SiteStateListener script/handler
    var frameInfos: Set<WKFrameInfo> = .init()
    for try await message in siteStateListenerMockMessageHandler {
      frameInfos.insert(message.frameInfo)
      let script = try ScriptFactory.shared.makeScript(
        for: .contentCosmetic(setup, proceduralActions: Set(proceduralFilters))
      )
      _ = try await withCheckedThrowingContinuation { continuation in
        viewController.webView.evaluateJavaScript(
          script.source,
          in: message.frameInfo,
          in: CosmeticFiltersScriptHandler.scriptSandbox
        ) { result in
          continuation.resume(with: result)
        }
      }
      // Expecting 2 frames (main frame & about:blank iframe)
      if frameInfos.count == 2 {
        break
      }
    }

    // Await the execution of the selectors message handler (so we know we already hid our elements)
    var count = 0
    for try await message in selectorsMessageHandler {
      count += 1
      if count == 1 {
        // Verify expected first pass elements. Some elements, like `test-delayed-has-text`,
        // are added later into the html for testing the mutation observer.
        do {
          let data = try JSONSerialization.data(withJSONObject: message.body)
          let dto = try JSONDecoder().decode(
            CosmeticFiltersScriptHandler.CosmeticFiltersDTO.self,
            from: data
          )

          if message.frameInfo.isMainFrame {
            for id in polledAggressiveIds {
              XCTAssertTrue(dto.data.ids.contains(id))
            }
            for id in polledStandardIds {
              XCTAssertTrue(dto.data.ids.contains(id))
            }
            for id in nestedIds {
              XCTAssertTrue(dto.data.ids.contains(id))
            }
          }
        } catch {
          XCTFail(String(describing: error))
        }
      }

      // We only care about the first script handler result for each frame
      if count == frameInfos.count {
        break
      }
    }

    let addDynamicElementsJavascript = """
        addElementForProceduralFilter(/* id */ 'test-delayed-has-text');
        addElementWithChildDynamically();
        triggerAltMutationObservationStrategy();
        // add element to hide using alt mutation observation strategy, but delayed
        // so it's not grouped with above mutation
        window.setTimeout(
          addElementForProceduralFilter, 
          /* delay */ 50,
          /* id */ 'test-alt-mutation-observation-strategy');
      """
    try await viewController.webView.evaluateJavaScript(addDynamicElementsJavascript)

    // Now wait for the pump which takes a few seconds (The pump unhides 1st party elements).
    try await Task.sleep(seconds: 5)

    // Execute a script that will test the cosmetic filters page
    let testURL = Bundle.module.url(forResource: "cosmetic-filter-tests", withExtension: "js")!
    let source = try String(contentsOf: testURL)
    _ = try await withCheckedThrowingContinuation { continuation in
      viewController.webView.evaluateJavaScript(
        source,
        in: nil,
        in: CosmeticFiltersScriptHandler.scriptSandbox
      ) { result in
        continuation.resume(with: result)
      }
    }

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
    XCTAssertTrue(resultsAfterPump?.upwardInt ?? false)
    XCTAssertTrue(resultsAfterPump?.upwardSelector ?? false)
    XCTAssertTrue(resultsAfterPump?.hasTextDisplayIsNone ?? false)
    XCTAssertTrue(resultsAfterPump?.hasDisplayIsNone ?? false)
    XCTAssertTrue(resultsAfterPump?.delayedHasTextHidden ?? false)
    if let delayedChildHasTextHidden = resultsAfterPump?.delayedChildHasTextHidden {
      // expecting 2 elements with the same id to be hidden
      XCTAssertEqual(delayedChildHasTextHidden.count, 2)
      XCTAssertTrue(delayedChildHasTextHidden.allSatisfy({ $0 }))
    } else {
      XCTFail("delayedChildHasTextHidden missing in results")
    }
    XCTAssertTrue(resultsAfterPump?.altMutationStrategyHidden ?? false)
    // Test for local frames
    XCTAssertTrue(resultsAfterPump?.localFrameElement ?? false)
  }

  @MainActor func testScriptlets() async throws {
    AdblockEngine.setDomainResolver()
    let engine = try AdblockEngine(
      rules: [
        "example.com##+js(set, sval, 1)"
      ].joined(separator: "\n")
    )
    try await engine.useResources(
      fromFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!
    )

    let viewController = MockScriptsViewController()
    viewController.loadViewIfNeeded()
    // create & inject the scriptlets user script before page load begins
    guard let frameURL = URL(string: "https://example.com"),
      let cosmeticFilterModel = try engine.cosmeticFilterModel(
        forFrameURL: frameURL
      ),
      case let source = cosmeticFilterModel.injectedScript,
      !source.isEmpty
    else {
      XCTFail("Injected script expected. Check test setup.")
      return
    }
    let configuration = UserScriptType.EngineScriptConfiguration(
      frameURL: frameURL,
      isMainFrame: false,
      source: source,
      order: 0,
      isDeAMPEnabled: false
    )
    let script = try viewController.scriptFactory.makeScript(for: .engineScript(configuration))
    viewController.add(userScript: script)

    // Load test html in the web view
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try String(contentsOf: htmlURL, encoding: .utf8)
    try await viewController.loadHTMLStringAndWait(htmlString)

    try await Task.sleep(seconds: 2)

    // check that the main frame & local frame (about:blank) have set value from scriptlet
    guard
      let mainFrameResult = try await withCheckedThrowingContinuation({ continuation in
        viewController.webView.evaluateJavaScript(
          "document.getElementById('scriptlet-main-div').innerText",
          in: nil,
          in: CosmeticFiltersScriptHandler.scriptSandbox
        ) { result in
          continuation.resume(with: result)
        }
      }) as? String,
      let localFrameResult = try await withCheckedThrowingContinuation({ continuation in
        viewController.webView.evaluateJavaScript(
          "document.getElementById('local-iframe').contentDocument.getElementById('scriptlet-local-frame-div').innerText",
          in: nil,
          in: CosmeticFiltersScriptHandler.scriptSandbox
        ) { result in
          continuation.resume(with: result)
        }
      }) as? String
    else {
      XCTFail("Elements should be available. Check test setup.")
      return
    }
    XCTAssertEqual(mainFrameResult, "First party body: 1")
    XCTAssertEqual(localFrameResult, "First local frame body: 1")
  }
}
