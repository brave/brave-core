// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
@testable import Brave

class AdBlockEngineManagerTests: XCTestCase {
  
  func testCompilationofResources() throws {
    let filterListDATURLs: [AdBlockEngineManager.Source: URL] = [
      // Default
      .adBlock:
        Bundle.module.url(forResource: "rs-ABPFilterParserData", withExtension: "dat")!,
      // Annoyances
      .filterList(uuid: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8"):
        Bundle.module.url(forResource: "rs-67E792D4-AE03-4D1A-9EDE-80E01C81F9B8", withExtension: "dat")!,
      // Cookie notice blocking
      .filterList(uuid: "AC023D22-AE88-4060-A978-4FEEEC4221693"):
        Bundle.module.url(forResource: "rs-AC023D22-AE88-4060-A978-4FEEEC4221693", withExtension: "dat")!
    ]
    
    let filterListURLs: [AdBlockEngineManager.Source: URL] = [
      // Default
      .adBlock:
        Bundle.module.url(forResource: "latest", withExtension: "txt")!,
      // Annoyances
      .filterList(uuid: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8"):
        Bundle.module.url(forResource: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8-latest", withExtension: "txt")!,
      // Cookie notice blocking
      .filterList(uuid: "AC023D22-AE88-4060-A978-4FEEEC4221693"):
        Bundle.module.url(forResource: "AC023D22-AE88-4060-A978-4FEEEC4221693-latest", withExtension: "txt")!
    ]
    
    let resourceURLs: [AdBlockEngineManager.Source: URL] = [
      // Default
      .adBlock:
        Bundle.module.url(forResource: "resources", withExtension: "json")!,
      // Annoyances
      .filterList(uuid: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8"):
        Bundle.module.url(forResource: "67E792D4-AE03-4D1A-9EDE-80E01C81F9B8-resources", withExtension: "json")!,
      // Cookie notice blocking
      .filterList(uuid: "AC023D22-AE88-4060-A978-4FEEEC4221693"):
        Bundle.module.url(forResource: "AC023D22-AE88-4060-A978-4FEEEC4221693-resources", withExtension: "json")!
    ]
    
    let expectation = expectation(description: "Compiled engine resources")
    let stats = AdBlockStats()
    let engineManager = AdBlockEngineManager(stats: stats)
    
    AdblockEngine.setDomainResolver(AdblockEngine.defaultDomainResolver)
    
    Task.detached {
      for (source, url) in filterListURLs {
        await engineManager.add(
          resource: .init(type: .ruleList, source: source),
          fileURL: url,
          version: nil
        )
      }
      
      for (source, url) in filterListDATURLs {
        await engineManager.add(
          resource: .init(type: .dat, source: source),
          fileURL: url,
          version: nil
        )
      }
      
      for (source, url) in resourceURLs {
        await engineManager.add(
          resource: .init(type: .jsonResources, source: source),
          fileURL: url,
          version: nil
        )
      }
      
      await engineManager.compileResources()
      
      Task { @MainActor in
        XCTAssertEqual(stats.cachedEngines.count, 3)
        let types = stats.makeEngineScriptTypes(frameURL: URL(string:  "https://stackoverflow.com")!, isMainFrame: true)
        // We should have no scripts injected
        XCTAssertEqual(types.count, 0)
        
        let types2 = stats.makeEngineScriptTypes(frameURL: URL(string:  "https://reddit.com")!, isMainFrame: true)
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
}
