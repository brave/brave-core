// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import WebKit
import XCTest

@testable import Brave

final class AdBlockGroupsManagerTests: XCTestCase {
  private static var sampleFilterListURL = Bundle.module.url(
    forResource: "iodkpdagapdfkphljnddpjlldadblomo",
    withExtension: "txt"
  )!

  @MainActor private lazy var ruleStore: WKContentRuleListStore = {
    let testBundle = Bundle.module
    let bundleURL = testBundle.bundleURL
    return WKContentRuleListStore(url: bundleURL)!
  }()

  /// Testing engine compilations and filter list managment
  @MainActor func testCompilation() async throws {
    AdblockEngine.setDomainResolver()

    // Given
    // A source provider and groups manager
    let fileInfos = [
      AdBlockEngineManager.FileInfo(
        filterListInfo: GroupedAdBlockEngine.FilterListInfo(
          source: .filterListURL(uuid: UUID().uuidString),
          version: "0"
        ),
        localFileURL: Self.sampleFilterListURL
      ),
      AdBlockEngineManager.FileInfo(
        filterListInfo: GroupedAdBlockEngine.FilterListInfo(
          source: .filterListURL(uuid: UUID().uuidString),
          version: "0"
        ),
        localFileURL: Self.sampleFilterListURL
      ),
    ]
    let sourceProvider = TestSourceProvider(fileInfos: fileInfos)
    let standardManager = AdBlockEngineManager(
      engineType: .standard,
      cacheFolderName: "test_standard"
    )
    let aggressiveManager = AdBlockEngineManager(
      engineType: .aggressive,
      cacheFolderName: "test_aggressive"
    )
    let groupsManager = AdBlockGroupsManager(
      standardManager: standardManager,
      aggressiveManager: aggressiveManager,
      contentBlockerManager: makeContentBlockingManager(),
      sourceProvider: sourceProvider
    )
    let resourcesInfo = GroupedAdBlockEngine.ResourcesInfo(
      localFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!,
      version: "0"
    )

    // When
    // Adding file info and enabling sources for only one engine
    sourceProvider.set(
      source: fileInfos[0].filterListInfo.source,
      enabled: true
    )
    groupsManager.updateIfNeeded(resourcesInfo: resourcesInfo)
    groupsManager.update(fileInfo: fileInfos[1], engineType: .standard, compileDelayed: false)
    groupsManager.update(fileInfos: fileInfos, engineType: .aggressive, compileDelayed: false)
    await groupsManager.compileEnginesIfNeeded()

    // Then
    // Only one engine is created
    var standardEngine = standardManager.engine
    var aggressiveEngine = aggressiveManager.engine
    var standardGroup = await standardEngine?.group
    var aggressiveGroup = await aggressiveEngine?.group
    var standardResources = await standardEngine?.resourcesInfo
    var aggressiveResources = await aggressiveEngine?.resourcesInfo
    XCTAssertNil(standardEngine)
    XCTAssertNotNil(aggressiveEngine)
    XCTAssertEqual(aggressiveResources, resourcesInfo)
    XCTAssertEqual(aggressiveGroup?.infos, [fileInfos[0].filterListInfo])

    // When
    // We enable sources and recompile the engine
    sourceProvider.set(
      source: fileInfos[1].filterListInfo.source,
      enabled: true
    )
    groupsManager.updateIfNeeded(resourcesInfo: resourcesInfo)
    await groupsManager.compileEnginesIfNeeded()

    // Then
    // All engines are created
    standardEngine = standardManager.engine
    aggressiveEngine = aggressiveManager.engine
    standardGroup = await standardEngine?.group
    aggressiveGroup = await aggressiveEngine?.group
    standardResources = await standardEngine?.resourcesInfo
    aggressiveResources = await aggressiveEngine?.resourcesInfo
    XCTAssertNotNil(standardEngine)
    XCTAssertNotNil(aggressiveEngine)
    XCTAssertEqual(standardResources, resourcesInfo)
    XCTAssertEqual(aggressiveResources, resourcesInfo)
    XCTAssertEqual(standardGroup?.infos, [fileInfos[1].filterListInfo])
    XCTAssertEqual(aggressiveGroup?.infos, fileInfos.map({ $0.filterListInfo }))
  }

  /// Partially testing expectations found in https://dev-pages.bravesoftware.com/filtering/index.html
  @MainActor func testBlocking() async throws {
    AdblockEngine.setDomainResolver()

    // Given
    // A source provider and groups manager
    let fileInfo = AdBlockEngineManager.FileInfo(
      filterListInfo: GroupedAdBlockEngine.FilterListInfo(
        source: .filterListURL(uuid: UUID().uuidString),
        version: "0"
      ),
      localFileURL: Self.sampleFilterListURL
    )
    let sourceProvider = TestSourceProvider(fileInfos: [fileInfo])
    let standardManager = AdBlockEngineManager(
      engineType: .standard,
      cacheFolderName: "test_standard"
    )
    let aggressiveManager = AdBlockEngineManager(
      engineType: .aggressive,
      cacheFolderName: "test_aggressive"
    )
    let groupsManager = AdBlockGroupsManager(
      standardManager: standardManager,
      aggressiveManager: aggressiveManager,
      contentBlockerManager: makeContentBlockingManager(),
      sourceProvider: sourceProvider
    )
    let resourcesInfo = GroupedAdBlockEngine.ResourcesInfo(
      localFileURL: Bundle.module.url(forResource: "resources", withExtension: "json")!,
      version: "0"
    )

    // When
    // Adding creating a standard engine
    sourceProvider.set(
      source: fileInfo.filterListInfo.source,
      enabled: true
    )
    groupsManager.updateIfNeeded(resourcesInfo: resourcesInfo)
    groupsManager.update(fileInfo: fileInfo, engineType: .standard, compileDelayed: false)
    await groupsManager.compileEnginesIfNeeded()

    let mainFrameURL = URL(string: "https://dev-pages.bravesoftware.com")!
    let domain = await MainActor.run {
      return Domain.getOrCreate(forUrl: mainFrameURL, persistent: false)
    }

    // Then
    // Should have certian script types
    let scriptTypes = await groupsManager.makeEngineScriptTypes(
      frameURL: URL(string: "https://dev-pages.bravesoftware.com/filtering/scriptlets.html")!,
      isMainFrame: true,
      isDeAmpEnabled: false,
      domain: domain
    )
    XCTAssertTrue(
      scriptTypes.contains(where: { scriptType in
        switch scriptType {
        case .engineScript(let configuration):
          return configuration.source.contains("window.BRAVE_TEST_VALUE")
        default:
          return false
        }
      })
    )

    // Then
    // Should return some filter models
    let cosmeticFilterModels = await groupsManager.cosmeticFilterModels(
      forFrameURL: mainFrameURL,
      domain: domain
    )
    XCTAssertFalse(cosmeticFilterModels.isEmpty)

    // Should block certain 3rd party content
    let sourceURL = URL(
      string: "https://dev-pages.bravesoftware.com/filtering/network-requests.html"
    )!
    let requestURL = URL(
      string: "https://dev-pages.brave.software/static/images/test.jpg?335962573013224749"
    )!
    var blockResult = await groupsManager.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: .image,
      domain: domain
    )
    XCTAssertTrue(blockResult)

    // Should not block certain 1sd party content
    let requestURL2 = URL(
      string: "https://dev-pages.bravesoftware.com/static/images/test.jpg?335962573013224749"
    )!
    blockResult = await groupsManager.shouldBlock(
      requestURL: requestURL2,
      sourceURL: sourceURL,
      resourceType: .image,
      domain: domain
    )
    XCTAssertFalse(blockResult)

    // When
    // Adding an aggressive filter list
    groupsManager.update(fileInfo: fileInfo, engineType: .aggressive, compileDelayed: false)
    await groupsManager.compileEnginesIfNeeded()

    // Then
    // Should block certain 1sd party content
    blockResult = await groupsManager.shouldBlock(
      requestURL: requestURL2,
      sourceURL: sourceURL,
      resourceType: .image,
      domain: domain
    )
    XCTAssertTrue(blockResult)
  }

  @MainActor private func makeContentBlockingManager() -> ContentBlockerManager {
    return ContentBlockerManager(ruleStore: ruleStore)
  }
}

class TestSourceProvider: AdBlockGroupsManager.SourceProvider {
  /// The sources that are enabled
  private var _enabledSources: Set<GroupedAdBlockEngine.Source>
  private var fileInfos: [AdBlockEngineManager.FileInfo] = []

  init(
    _enabledSources: Set<GroupedAdBlockEngine.Source> = [],
    fileInfos: [AdBlockEngineManager.FileInfo] = []
  ) {
    self._enabledSources = _enabledSources
    self.fileInfos = fileInfos
  }

  func set(source: GroupedAdBlockEngine.Source, enabled: Bool) {
    if enabled {
      _enabledSources.insert(source)
    } else {
      _enabledSources.remove(source)
    }
  }

  var enabledSources: [GroupedAdBlockEngine.Source] {
    return fileInfos.compactMap { fileInfo in
      guard _enabledSources.contains(fileInfo.filterListInfo.source) else { return nil }
      return fileInfo.filterListInfo.source
    }
  }

  /// Return all engabled sources for the given engine type
  func enabledSources(
    for engineType: GroupedAdBlockEngine.EngineType
  ) -> [GroupedAdBlockEngine.Source] {
    return enabledSources
  }

  func legacyCacheFiles(
    for engineType: Brave.GroupedAdBlockEngine.EngineType
  ) -> [Brave.AdBlockEngineManager.FileInfo] {
    return []
  }
}
