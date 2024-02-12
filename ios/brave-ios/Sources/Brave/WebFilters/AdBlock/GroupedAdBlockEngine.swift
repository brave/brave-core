// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import os

/// An object that wraps around an `AdblockEngine` and caches some results
/// and ensures information is always returned on the correct thread on the engine.
public class GroupedAdBlockEngine {
  public enum Source: Codable, Hashable, CustomDebugStringConvertible {
    case filterList(componentId: String, uuid: String)
    case filterListURL(uuid: String)

    public var debugDescription: String {
      switch self {
      case .filterList(let componentId, _): return "filterList(\(componentId))"
      case .filterListURL(let uuid): return "filterListURL(\(uuid))"
      }
    }
  }

  public enum FileType: Codable, Hashable, CustomDebugStringConvertible {
    case text, data

    public var debugDescription: String {
      switch self {
      case .text: return "txt"
      case .data: return "dat"
      }
    }
  }

  public enum EngineType: Hashable, CaseIterable {
    case standard
    case aggressive

    var isAlwaysAggressive: Bool {
      switch self {
      case .standard: return false
      case .aggressive: return true
      }
    }
  }

  public struct FilterListInfo: Codable, Hashable, Equatable, CustomDebugStringConvertible {
    let source: GroupedAdBlockEngine.Source
    let localFileURL: URL
    let version: String

    public var debugDescription: String {
      return "`\(source.debugDescription)` v\(version)"
    }
  }

  public struct FilterListGroup: Hashable, Equatable, CustomDebugStringConvertible {
    let infos: [FilterListInfo]
    let localFileURL: URL
    let fileType: GroupedAdBlockEngine.FileType

    public var debugDescription: String {
      return infos.map({ $0.debugDescription }).joined(separator: ", ")
    }
  }

  public struct ResourcesInfo: Hashable, Equatable {
    let localFileURL: URL
    let version: String
  }

  static let signpost = OSSignposter(logger: ContentBlockerManager.log)
  /// We cache the models so that they load faster when we need to poll information about the frame
  private var cachedCosmeticFilterModels = FifoDict<URL, CosmeticFilterModel?>()
  /// We cache the models so that they load faster when doing stats tracking or request blocking
  private var cachedShouldBlockResult = FifoDict<String, Bool>()
  /// We cache the user scripts so that they load faster on refreshes and back and forth
  private var cachedFrameScriptTypes = FifoDict<URL, Set<UserScriptType>>()

  private let engine: AdblockEngine

  let type: EngineType
  let group: FilterListGroup
  private(set) var resourcesInfo: ResourcesInfo?

  init(engine: AdblockEngine, group: FilterListGroup, type: EngineType) {
    self.engine = engine
    self.group = group
    self.type = type
  }

  /// Return the selectors that need to be hidden given the frameURL, ids and classes
  func selectorsForCosmeticRules(
    frameURL: URL,
    ids: [String],
    classes: [String]
  ) throws -> Set<String>? {
    let model = try self.cosmeticFilterModel(forFrameURL: frameURL)

    let selectors = try self.engine.stylesheetForCosmeticRulesIncluding(
      classes: classes,
      ids: ids,
      exceptions: model?.exceptions ?? []
    )

    return Set(selectors)
  }

  /// Return a cosmetic filter modelf or the given frameURL
  ///
  /// - Warning: The caller is responsible for syncing on the `serialQueue`
  func cosmeticFilterModel(forFrameURL frameURL: URL) throws -> CosmeticFilterModel? {
    if let result = self.cachedCosmeticFilterModels.getElement(frameURL) {
      return result
    }

    let model = try self.engine.cosmeticFilterModel(forFrameURL: frameURL)
    self.cachedCosmeticFilterModels.addElement(model, forKey: frameURL)
    return model
  }

  /// Checks the general and regional engines to see if the request should be blocked
  func shouldBlock(
    requestURL: URL,
    sourceURL: URL,
    resourceType: AdblockEngine.ResourceType,
    isAggressiveMode: Bool
  ) -> Bool {
    let key = [requestURL.absoluteString, sourceURL.absoluteString, resourceType.rawValue].joined(
      separator: "_"
    )

    if let cachedResult = cachedShouldBlockResult.getElement(key) {
      return cachedResult
    }

    let shouldBlock = engine.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType,
      isAggressive: isAggressiveMode || self.type.isAlwaysAggressive
    )

    cachedShouldBlockResult.addElement(shouldBlock, forKey: key)
    return shouldBlock
  }

  /// This returns all the user script types for the given frame
  @MainActor func makeEngineScriptTypes(
    frameURL: URL,
    isMainFrame: Bool,
    domain: Domain,
    index: Int
  ) throws -> Set<UserScriptType> {
    if let userScriptTypes = cachedFrameScriptTypes.getElement(frameURL) {
      return userScriptTypes
    }

    // Add the selectors poller scripts for this frame
    var userScriptTypes: Set<UserScriptType> = []

    if let source = try cosmeticFilterModel(forFrameURL: frameURL)?.injectedScript, !source.isEmpty
    {
      let configuration = UserScriptType.EngineScriptConfiguration(
        frameURL: frameURL,
        isMainFrame: isMainFrame,
        source: source,
        order: index,
        isDeAMPEnabled: Preferences.Shields.autoRedirectAMPPages.value
      )

      userScriptTypes.insert(.engineScript(configuration))
    }

    cachedFrameScriptTypes.addElement(userScriptTypes, forKey: frameURL)
    return userScriptTypes
  }

  /// Clear the caches.
  func clearCaches() {
    cachedCosmeticFilterModels = FifoDict()
    cachedShouldBlockResult = FifoDict()
    cachedFrameScriptTypes = FifoDict()
  }

  func useResources(from info: ResourcesInfo) throws {
    try engine.useResources(fromFileURL: info.localFileURL)
    resourcesInfo = info
  }

  /// Create an engine from the given resources
  public static func compile(
    group: FilterListGroup,
    type: EngineType
  ) throws -> GroupedAdBlockEngine {
    let signpostID = Self.signpost.makeSignpostID()
    let state = Self.signpost.beginInterval(
      "compileEngine",
      id: signpostID,
      "\(group.debugDescription)"
    )

    do {
      let engine = try makeEngine(from: group)
      Self.signpost.endInterval("compileEngine", state)
      return GroupedAdBlockEngine(engine: engine, group: group, type: type)
    } catch {
      Self.signpost.endInterval("compileEngine", state, "\(error.localizedDescription)")
      throw error
    }
  }

  private static func makeEngine(from group: FilterListGroup) throws -> AdblockEngine {
    switch group.fileType {
    case .data:
      return try AdblockEngine(serializedData: Data(contentsOf: group.localFileURL))
    case .text:
      return try AdblockEngine(rules: String(contentsOf: group.localFileURL))
    }
  }
}
