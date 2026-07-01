// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Web
import os.log

extension TabDataValues {
  private struct CosmeticFilteringTabHelperKey: TabDataKey {
    static var defaultValue: CosmeticFilteringTabHelper?
  }
  public var cosmeticFilteringTabHelper: CosmeticFilteringTabHelper? {
    get { self[CosmeticFilteringTabHelperKey.self] }
    set { self[CosmeticFilteringTabHelperKey.self] = newValue }
  }
}

public class CosmeticFilteringTabHelper {

  private weak var tab: (any TabState)?
  /// Cached standard selectors. Key is the URL's `baseDomain`.
  private var hiddenStandardSelectors: [String: Set<String>] = [:]
  /// Cached aggressive selectors. Key is the URL's `baseDomain`.
  private var hiddenAggressiveSelectors: [String: Set<String>] = [:]

  public init(
    tab: some TabState
  ) {
    self.tab = tab
  }

  /// Removes all selectors from the selectors caches.
  func resetSelectorsCache() {
    hiddenStandardSelectors.removeAll()
    hiddenAggressiveSelectors.removeAll()
  }

  /// Get the cached selectors for the given URL.
  func cachedSelectors(for url: URL) -> (standard: Set<String>, aggressive: Set<String>)? {
    guard let baseDomain = url.baseDomain else {
      return nil
    }
    return (hiddenStandardSelectors[baseDomain] ?? [], hiddenAggressiveSelectors[baseDomain] ?? [])
  }

  /// Cache the given selectors for the given URL.
  func cacheSelectors(
    for url: URL,
    standardSelectors: Set<String>,
    aggressiveSelectors: Set<String>
  ) {
    guard let baseDomain = url.baseDomain else {
      return
    }
    var cachedStandardSelectors = hiddenStandardSelectors[baseDomain] ?? .init()
    cachedStandardSelectors.formUnion(standardSelectors)
    var cachedAggressiveSelectors = hiddenAggressiveSelectors[baseDomain] ?? .init()
    cachedAggressiveSelectors.formUnion(aggressiveSelectors)
    hiddenStandardSelectors[baseDomain] = cachedStandardSelectors
    hiddenAggressiveSelectors[baseDomain] = cachedAggressiveSelectors
  }

  /// Combine new selectors with the cached selectors for the tab's visibleURL.
  @MainActor func standardAndAggressiveSelectors(
    from models: [AdBlockGroupsManager.CosmeticFilterModelTuple]
  ) -> (Set<String>, Set<String>) {
    var cachedStandardSelectors: Set<String> = .init()
    var cachedAggressiveSelectors: Set<String> = .init()
    if let mainFrameURL = tab?.currentPageData?.mainFrameURL,
      let (standard, aggressive) = cachedSelectors(for: mainFrameURL)
    {
      cachedStandardSelectors = standard
      cachedAggressiveSelectors = aggressive
    }
    var standardSelectors: Set<String> = cachedStandardSelectors
    var aggressiveSelectors: Set<String> = cachedAggressiveSelectors
    for modelTuple in models {
      if modelTuple.isAlwaysAggressive {
        aggressiveSelectors = aggressiveSelectors.union(modelTuple.model.hideSelectors)
      } else {
        standardSelectors = standardSelectors.union(modelTuple.model.hideSelectors)
      }
    }
    return (standardSelectors, aggressiveSelectors)
  }

  /// Given a `frameURL`, retrieves the cosmetic filtering setup and procedural
  /// actions for the frame using the Tab's main-frame shield level.
  /// - returns a tuple containing the `ContentCosmeticSetup` and a
  /// `Set<String>` of the procedural actions, or nil if Shields is disabled or
  /// unavailable
  @MainActor func cosmeticFilteringSetup(
    for frameURL: URL
  ) async -> (UserScriptType.ContentCosmeticSetup, Set<String>)? {
    guard let tab = tab,
      let mainFrameURL = tab.currentPageData?.mainFrameURL,
      let braveShieldsHelper = tab.braveShieldsHelper,
      // shield level is determined by the main frame
      case let mainFrameShieldLevel = braveShieldsHelper.shieldLevel(
        for: mainFrameURL,
        considerAllShieldsOption: true
      ),
      mainFrameShieldLevel.isEnabled,
      mainFrameURL.isWebPage(includeDataURIs: false)
    else {
      return nil
    }

    let models = await AdBlockGroupsManager.shared.cosmeticFilterModels(
      forFrameURL: frameURL,
      isAdBlockEnabled: mainFrameShieldLevel.isEnabled
    )
    let (standardSelectors, aggressiveSelectors) = standardAndAggressiveSelectors(from: models)

    var proceduralActions: Set<String> = []
    for modelTuple in models {
      proceduralActions = proceduralActions.union(modelTuple.model.proceduralActions)
    }

    let setup = UserScriptType.ContentCosmeticSetup(
      hideFirstPartyContent: mainFrameShieldLevel.isAggressive,
      genericHide: models.contains { $0.model.genericHide },
      firstSelectorsPollingDelayMs: nil,
      switchToSelectorsPollingThreshold: 1000,
      fetchNewClassIdRulesThrottlingMs: 100,
      aggressiveSelectors: aggressiveSelectors,
      standardSelectors: standardSelectors
    )
    return (setup, proceduralActions)
  }

  /// Given a `frameURL` and `Set<String>`'s of `ids` and `classes`, determines
  /// which ids and classes should be hidden for the frame using the Tab's
  /// main-frame shield level.
  /// - returns a tuple containing a `Set<String>` of standard selectors and
  /// aggressive selectors to hide, or nil if Shields is disabled or
  /// unavailable
  @MainActor public func selectorsToHide(
    for frameURL: URL,
    ids: Set<String>,
    classes: Set<String>,
  ) async -> (Set<String>, Set<String>)? {
    guard let tab = tab,
      let tabPageData = tab.currentPageData,
      let braveShieldsHelper = tab.braveShieldsHelper,
      // shield level is determined by the main frame
      case let mainFrameShieldLevel = braveShieldsHelper.shieldLevel(
        for: tabPageData.mainFrameURL,
        considerAllShieldsOption: true
      ),
      mainFrameShieldLevel.isEnabled,
      tabPageData.mainFrameURL.isWebPage(includeDataURIs: false)
    else {
      return nil
    }
    let cachedEngines = AdBlockGroupsManager.shared.cachedEngines(
      isAdBlockEnabled: mainFrameShieldLevel.isEnabled
    )

    let selectorArrays = await cachedEngines.asyncCompactMap {
      cachedEngine -> (selectors: Set<String>, isAlwaysAggressive: Bool)? in
      do {
        guard
          let selectors = try await cachedEngine.selectorsForCosmeticRules(
            frameURL: frameURL,
            ids: Array(ids),
            classes: Array(classes)
          )
        else {
          return nil
        }

        return await (selectors, cachedEngine.type.isAlwaysAggressive)
      } catch {
        Logger.module.error("\(error.localizedDescription)")
        return nil
      }
    }

    var standardSelectors: Set<String> = []
    var aggressiveSelectors: Set<String> = []
    for tuple in selectorArrays {
      if tuple.isAlwaysAggressive {
        aggressiveSelectors = aggressiveSelectors.union(tuple.selectors)
      } else {
        standardSelectors = standardSelectors.union(tuple.selectors)
      }
    }

    // cache blocked selectors
    cacheSelectors(
      for: tabPageData.mainFrameURL,
      standardSelectors: standardSelectors,
      aggressiveSelectors: aggressiveSelectors
    )
    return (standardSelectors, aggressiveSelectors)
  }
}
