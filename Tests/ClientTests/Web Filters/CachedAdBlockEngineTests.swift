// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
import Data
@testable import Brave

final class CachedAdBlockEngineTests: XCTestCase {
  func test3rdPartyCheck() throws {
    let engine = try AdblockEngine(rules: [
      "||brave.com/path",
      "||brave.com/other-path",
      "@@||brave.com/path$~third-party"
    ].joined(separator: "\n"))
    
    XCTAssertFalse(
      engine.shouldBlock(
        requestURL: URL(string: "https://subdomain.brave.com/path")!,
        sourceURL: URL(string: "https://brave.com")!, resourceType: .xmlhttprequest,
        isAggressive: false
      ),
      "We should not be blocking requests in the same domain on standard mode, regardless of the rules"
    )
    XCTAssertFalse(
      engine.shouldBlock(
        requestURL: URL(string: "https://subdomain.brave.com/path")!,
        sourceURL: URL(string: "https://brave.com")!, resourceType: .xmlhttprequest,
        isAggressive: true
      ),
      "We should not be blocking this request even on aggressive mode because of the exception rule"
    )
    XCTAssertFalse(
      engine.shouldBlock(
        requestURL: URL(string: "https://subdomain.brave.com/other-path")!,
        sourceURL: URL(string: "https://brave.com")!, resourceType: .xmlhttprequest,
        isAggressive: false
      ),
      "We should not be blocking requests in the same domain on standard mode, regardless of the rules"
    )
    XCTAssertTrue(
      engine.shouldBlock(
        requestURL: URL(string: "https://subdomain.brave.com/other-path")!,
        sourceURL: URL(string: "https://brave.com")!, resourceType: .xmlhttprequest,
        isAggressive: true
      ),
      "We should be blocking this request on aggressive mode because of the blocking rule without an exception"
    )
  }
  
  func testEngineMemoryManagment() throws {
    AdblockEngine.setDomainResolver()
    var engine: AdblockEngine? = AdblockEngine()
    weak var weakEngine: AdblockEngine? = engine
    
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!, version: "bundled"
    )
    
    let filterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: .adBlock,
      localFileURL: Bundle.module.url(forResource: "cdbbhgbmjhfnhnmgeddbliobbofkgdhe", withExtension: "txt")!,
      version: "bundled", fileType: .text
    )
    
    var cachedEngine: CachedAdBlockEngine? = CachedAdBlockEngine(
      engine: engine!, filterListInfo: filterListInfo, resourcesInfo: resourcesInfo,
      serialQueue: DispatchQueue(label: "test"), isAlwaysAggressive: false
    )
    
    XCTAssertNotNil(cachedEngine)
    XCTAssertNotNil(weakEngine)
    
    engine = nil
    cachedEngine = nil
    
    XCTAssertNil(engine)
    XCTAssertNil(weakEngine)
    XCTAssertNil(cachedEngine)
  }
  
  func testCompilationofResources() throws {
    let textFilterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: .filterList(componentId: "cdbbhgbmjhfnhnmgeddbliobbofkgdhe"),
      localFileURL: Bundle.module.url(forResource: "cdbbhgbmjhfnhnmgeddbliobbofkgdhe", withExtension: "txt")!,
      version: "bundled", fileType: .text
    )
    let datFilterListInfo = CachedAdBlockEngine.FilterListInfo(
      source: .adBlock,
      localFileURL: Bundle.module.url(forResource: "cffkpbalmllkdoenhmdmpbkajipdjfam", withExtension: "dat")!,
      version: "bundled", fileType: .dat
    )
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!, version: "bundled"
    )
    
    let expectation = expectation(description: "Compiled engine resources")
    AdblockEngine.setDomainResolver()
    
    Task { @MainActor in
      let filterListInfos = [
        textFilterListInfo, datFilterListInfo
      ]
      
      await filterListInfos.asyncConcurrentForEach { filterListInfo in
        do {
          let engine = try CachedAdBlockEngine.compile(
            filterListInfo: filterListInfo, resourcesInfo: resourcesInfo, isAlwaysAggressive: false
          )
          
          let url = URL(string:  "https://stackoverflow.com")!
          
          let domain = await MainActor.run {
            return Domain.getOrCreate(forUrl: url, persistent: false)
          }
          
          let sameDomainTypes = try await engine.makeEngineScriptTypes(
            frameURL: url, isMainFrame: true, domain: domain, index: 0
          )
          
          // We should have no scripts injected
          XCTAssertEqual(sameDomainTypes.count, 0)
          
          if engine.filterListInfo == datFilterListInfo {
            // This engine file contains some scriplet rules so we can test this part is working
            let crossDomainTypes = try await engine.makeEngineScriptTypes(
              frameURL: URL(string:  "https://reddit.com")!, isMainFrame: true, domain: domain, index: 0
            )
            // We should have 1 engine script injected
            XCTAssertEqual(crossDomainTypes.count, 1)
            XCTAssertTrue(crossDomainTypes.contains(where: { scriptType in
              switch scriptType {
              case .engineScript: return true
              default: return false
              }
            }))
          }
        } catch {
          XCTFail(error.localizedDescription)
        }
      }
      
      expectation.fulfill()
    }
    
    waitForExpectations(timeout: 10)
  }
  
  func testPerformance() throws {
    // Given
    // Ad block data and an engine manager
    let sampleFilterListURL = Bundle.module.url(forResource: "cdbbhgbmjhfnhnmgeddbliobbofkgdhe", withExtension: "txt")!
    let resourcesInfo = CachedAdBlockEngine.ResourcesInfo(
      localFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!, version: "bundled"
    )
    
    // Then
    // Measure performance
    let options = XCTMeasureOptions()
    options.iterationCount = 10
    
    measure(metrics: [XCTClockMetric(), XCTCPUMetric(), XCTMemoryMetric()], options: options) {
      let uuid = UUID().uuidString
      
      let filterListInfo = CachedAdBlockEngine.FilterListInfo(
        source: .filterList(componentId: uuid),
        localFileURL: sampleFilterListURL,
        version: "bundled", fileType: .text
      )
      
      do {
        _ = try CachedAdBlockEngine.compile(
          filterListInfo: filterListInfo, resourcesInfo: resourcesInfo, isAlwaysAggressive: false
        )
      } catch {
        XCTFail(error.localizedDescription)
      }
    }
  }
}
