// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

final class AdBlockEngineManagerTests: XCTestCase {
  func testEngineManager() async throws {
    // Given
    // Engine manager and file info and resources info
    let engineManager = await AdBlockEngineManager(
      engineType: .standard,
      cacheFolderName: "test-standard"
    )
    let sampleFilterListURL = Bundle.module.url(
      forResource: "iodkpdagapdfkphljnddpjlldadblomo",
      withExtension: "txt"
    )!
    let resourcesInfo = GroupedAdBlockEngine.ResourcesInfo(
      localFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!,
      version: "0"
    )
    let fileInfos = [
      AdBlockEngineManager.FileInfo(
        filterListInfo: GroupedAdBlockEngine.FilterListInfo(
          source: .filterListURL(uuid: UUID().uuidString),
          version: "0"
        ),
        localFileURL: sampleFilterListURL
      ),
      AdBlockEngineManager.FileInfo(
        filterListInfo: GroupedAdBlockEngine.FilterListInfo(
          source: .filterListURL(uuid: UUID().uuidString),
          version: "0"
        ),
        localFileURL: sampleFilterListURL
      ),
    ]
    let sources = fileInfos.map({ $0.filterListInfo.source })

    // When
    // Adding file info
    for fileInfo in fileInfos {
      await engineManager.add(fileInfo: fileInfo)
    }

    // Then
    // Needs compile returns true and there is no engine
    var needsCompile = await engineManager.checkNeedsCompile(for: fileInfos)
    var engine = await engineManager.engine
    XCTAssertTrue(needsCompile)
    XCTAssertNil(engine)

    // When
    // Loaded from cache
    var loadedFromCache = await engineManager.loadFromCache(resourcesInfo: resourcesInfo)

    // Then
    // We don't have any engine
    engine = await engineManager.engine
    XCTAssertNil(engine)
    XCTAssertFalse(loadedFromCache)

    // When 2
    // We compile engine
    await engineManager.compileImmediatelyIfNeeded(
      for: sources,
      resourcesInfo: resourcesInfo
    )

    // Then
    // Needs compile returns false and engine is correctly created
    needsCompile = await engineManager.checkNeedsCompile(for: fileInfos)
    engine = await engineManager.engine
    let compiledResources = await engine?.resourcesInfo
    let group = await engine?.group
    XCTAssertFalse(needsCompile)
    XCTAssertNotNil(engine)
    XCTAssertEqual(group?.infos, fileInfos.map({ $0.filterListInfo }))
    XCTAssertEqual(group?.fileType, .text)
    XCTAssertEqual(compiledResources, resourcesInfo)

    // When
    // We load from cache using another manager with the same cache folder name
    let engineManager2 = await AdBlockEngineManager(
      engineType: .standard,
      cacheFolderName: "test-standard"
    )
    loadedFromCache = await engineManager2.loadFromCache(resourcesInfo: resourcesInfo)

    // Then
    // We have a cached engine
    engine = await engineManager2.engine
    let cacheGroup = await engine?.group
    let expectedURL = group?.localFileURL.deletingPathExtension().appendingPathExtension("dat")
    XCTAssertTrue(loadedFromCache)
    XCTAssertNotNil(engine)
    XCTAssertEqual(cacheGroup?.localFileURL, expectedURL)
    XCTAssertEqual(cacheGroup?.infos, fileInfos.map({ $0.filterListInfo }))
    XCTAssertEqual(cacheGroup?.fileType, .data)
    try await engineManager2.deleteCachedEngine()

    // When
    // We load from cache using a third manager with the same cache folder name
    let engineManager3 = await AdBlockEngineManager(
      engineType: .standard,
      cacheFolderName: "test-standard"
    )
    loadedFromCache = await engineManager3.loadFromCache(resourcesInfo: resourcesInfo)

    // Then
    // There is no cached engine because we deleted it previously
    engine = await engineManager3.engine
    XCTAssertFalse(loadedFromCache)
    XCTAssertNil(engine)
  }
}
