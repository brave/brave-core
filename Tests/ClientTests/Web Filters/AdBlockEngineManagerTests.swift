// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
import Data
@testable import Brave

class AdBlockEngineManagerTests: XCTestCase {
  
  func testCompilationofResources() throws {
    let filterListDATURLs: [AdBlockEngineManager.Source: URL] = [
      // Default
      .adBlock:
        Bundle.module.url(forResource: "rs-ABPFilterParserData", withExtension: "dat")!,
      // Annoyances
      .filterList(uuid: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8", isAlwaysAggressive: false):
        Bundle.module.url(forResource: "rs-67E792D4-AE03-4D1A-9EDE-80E01C81F9B8", withExtension: "dat")!,
      // Cookie notice blocking
      .filterList(uuid: "AC023D22-AE88-4060-A978-4FEEEC4221693", isAlwaysAggressive: false):
        Bundle.module.url(forResource: "rs-AC023D22-AE88-4060-A978-4FEEEC4221693", withExtension: "dat")!
    ]
    
    let filterListURLs: [AdBlockEngineManager.Source: URL] = [
      // Default
      .adBlock:
        Bundle.module.url(forResource: "latest", withExtension: "txt")!,
      // Annoyances
      .filterList(uuid: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8", isAlwaysAggressive: false):
        Bundle.module.url(forResource: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8-latest", withExtension: "txt")!,
      // Cookie notice blocking
      .filterList(uuid: "AC023D22-AE88-4060-A978-4FEEEC4221693", isAlwaysAggressive: false):
        Bundle.module.url(forResource: "AC023D22-AE88-4060-A978-4FEEEC4221693-latest", withExtension: "txt")!
    ]
    
    let resourceURLs: [AdBlockEngineManager.Source: URL] = [
      // Default
      .adBlock:
        Bundle.module.url(forResource: "resources", withExtension: "json")!,
      // Annoyances
      .filterList(uuid: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8", isAlwaysAggressive: false):
        Bundle.module.url(forResource: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8-resources", withExtension: "json")!,
      // Cookie notice blocking
      .filterList(uuid: "AC023D22-AE88-4060-A978-4FEEEC4221693", isAlwaysAggressive: false):
        Bundle.module.url(forResource: "AC023D22-AE88-4060-A978-4FEEEC4221693-resources", withExtension: "json")!
    ]
    
    let expectation = expectation(description: "Compiled engine resources")
    let stats = AdBlockStats()
    let engineManager = AdBlockEngineManager(stats: stats)
    
    AdblockEngine.setDomainResolver(AdblockEngine.defaultDomainResolver)
    
    Task.detached {
      for (index, (source, url)) in filterListURLs.enumerated() {
        await engineManager.add(
          resource: .init(type: .ruleList, source: source),
          fileURL: url,
          version: "1.0",
          relativeOrder: index
        )
      }
      
      for (index, (source, url)) in filterListDATURLs.enumerated() {
        await engineManager.add(
          resource: .init(type: .dat, source: source),
          fileURL: url,
          version: "1.0",
          relativeOrder: index + filterListURLs.count
        )
      }
      
      for (index, (source, url)) in resourceURLs.enumerated() {
        await engineManager.add(
          resource: .init(type: .jsonResources, source: source),
          fileURL: url,
          version: "1.0",
          relativeOrder: index + filterListURLs.count + filterListDATURLs.count
        )
      }
      
      await engineManager.compileResources()
      
      Task { @MainActor in
        let url = URL(string:  "https://stackoverflow.com")!
        let domain = Domain.getOrCreate(forUrl: url, persistent: false)
        let cachedEngines = stats.cachedEngines(for: domain)
        XCTAssertEqual(cachedEngines.count, 3)
        
        let types = await stats.makeEngineScriptTypes(frameURL: url, isMainFrame: true, domain: domain)
        // We should have no scripts injected
        XCTAssertEqual(types.count, 0)
        
        let url2 = URL(string:  "https://stackoverflow.com")!
        let domain2 = Domain.getOrCreate(forUrl: url2, persistent: false)
        let types2 = await stats.makeEngineScriptTypes(frameURL: URL(string:  "https://reddit.com")!, isMainFrame: true, domain: domain2)
        // We should have 1 engine script injected
        XCTAssertEqual(types2.count, 1)
        XCTAssertTrue(types2.contains(where: { scriptType in
          switch scriptType {
          case .engineScript: return true
          default: return false
          }
        }))
        expectation.fulfill()
      }
    }
    
    waitForExpectations(timeout: 10)
  }
  
  func testPerformance() throws {
    AdblockEngine.setDomainResolver(AdblockEngine.defaultDomainResolver)
    // Given
    // Ad block data and an engine manager
    let sampleAdBlockDatURL = Bundle.module.url(forResource: "rs-ABPFilterParserData", withExtension: "dat")!
    let sampleResourceURL = Bundle.module.url(forResource: "resources", withExtension: "json")!
    let engineManager = AdBlockEngineManager(stats: AdBlockStats())
    
    // When
    // Added number of resources to the engine manager
    let numberOfEngines = 10
    let setupExpectation = expectation(description: "Compiled engine resources")
    
    Task {
      for index in (0..<numberOfEngines) {
        let uuid = UUID().uuidString
        
        await engineManager.add(
          resource: .init(type: .dat, source: .filterList(uuid: uuid, isAlwaysAggressive: false)),
          fileURL: sampleAdBlockDatURL,
          version: "1.0",
          relativeOrder: index
        )
        
        await engineManager.add(
          resource: .init(type: .jsonResources, source: .filterList(uuid: uuid, isAlwaysAggressive: false)),
          fileURL: sampleResourceURL,
          version: "1.0",
          relativeOrder: index
        )
      }
      
      setupExpectation.fulfill()
    }
    
    wait(for: [setupExpectation], timeout: 10)
    
    // Then
    // Measure performance
    let options = XCTMeasureOptions()
    options.iterationCount = 10
    
    measure(metrics: [XCTClockMetric(), XCTCPUMetric(), XCTMemoryMetric()], options: options) {
      let exp = expectation(description: "Finished")

      Task {
        await engineManager.compileResources()
        exp.fulfill()
      }
      
      wait(for: [exp], timeout: 20)
    }
  }
}
