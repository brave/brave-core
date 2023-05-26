// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
import SnapKit
import CryptoKit

@testable import Brave

final class ScriptExecutionTests: XCTestCase {
  struct FarblingTestDTO: Decodable {
    let voiceNames: [String]
    let pluginNames: [String]
    let hardwareConcurrency: Int
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
    try await viewController.loadHTMLString(htmlString)
    
    // Then
    // Await the script handler and checks it's contents
    var foundMessage: SiteStateListenerScriptHandler.MessageDTO?
    for await message in stream {
      do {
        let data = try JSONSerialization.data(withJSONObject: message.body)
        foundMessage = try JSONDecoder().decode(SiteStateListenerScriptHandler.MessageDTO.self, from: data)
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
    let testScript = WKUserScript(source: source, injectionTime: .atDocumentEnd, forMainFrameOnly: true, in: .page)
    farbledViewController.add(userScript: testScript)
    controlViewController.add(userScript: testScript)
    
    // Load the sample htmls page and await the first page load result
    let htmlURL = Bundle.module.url(forResource: "index", withExtension: "html")!
    let htmlString = try! String(contentsOf: htmlURL, encoding: .utf8)
    async let load1: Void = controlViewController.loadHTMLString(htmlString)
    async let load2: Void = farbledViewController.loadHTMLString(htmlString)
    _ = try await (load1, load2)
    
    // Then
    // Await the results
    var farblingResult: FarblingTestDTO?
    for await message in stream {
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
    for await message in controlStream {
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
}
