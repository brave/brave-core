// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import os.log

/// An object that wraps around an `AdblockEngine` and caches some results
/// and ensures information is always returned on the correct thread on the engine.
public actor GroupedAdBlockEngine {
  public enum Source: Codable, Hashable, CustomDebugStringConvertible {
    case filterList(componentId: String)
    case filterListURL(uuid: String)
    case filterListText
    case slimList

    public var debugDescription: String {
      switch self {
      case .filterList(let componentId): return componentId
      case .filterListURL(let uuid): return uuid
      case .filterListText: return "filter-list-text"
      case .slimList: return "slim-list"
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

  /// The type of engine (`standard` or `aggressive`) which determines wether or not 1st party content will be blocked.
  ///
  /// Aggressive engines will block 1st party content whereas the standard engine will not
  public enum EngineType: Hashable, CaseIterable, CustomDebugStringConvertible {
    case standard
    case aggressive

    /// Tells us if this engine is always aggressive or if we need to switch between standard and aggressive
    var isAlwaysAggressive: Bool {
      switch self {
      case .standard: return false
      case .aggressive: return true
      }
    }

    /// Tells us wether or not the content blockers should be combined for this type
    ///
    /// - Note: This is only possible for the default (i.e. `standard`) engine
    /// as we control the filter lists for this type and we can guarantee they don't surpas 150k network rules.
    var combineContentBlockers: Bool {
      switch self {
      case .standard: return true
      case .aggressive: return false
      }
    }

    public var debugDescription: String {
      switch self {
      case .aggressive: return "aggressive"
      case .standard: return "standard"
      }
    }
  }

  public struct FilterListInfo: Codable, Hashable, Equatable, CustomDebugStringConvertible {
    let source: GroupedAdBlockEngine.Source
    let version: String

    public var debugDescription: String {
      return "`\(source.debugDescription)` v\(version)"
    }
  }

  public struct FilterListGroup: Hashable, Equatable {
    let infos: [FilterListInfo]
    let localFileURL: URL
    let fileType: GroupedAdBlockEngine.FileType

    public func makeDebugDescription(for engineType: GroupedAdBlockEngine.EngineType) -> String {
      return infos.enumerated()
        .map({
          let string = " #\($0) \($1.debugDescription)"
          if $1.source.onlyExceptions(for: engineType) {
            return "\(string) (exceptions)"
          } else {
            return string
          }
        })
        .joined(separator: "\n")
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
    let key = [
      requestURL.absoluteString, sourceURL.absoluteString, resourceType.rawValue,
      "\(isAggressiveMode)",
    ].joined(
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
  func makeEngineScriptTypes(
    frameURL: URL,
    isMainFrame: Bool,
    isDeAmpEnabled: Bool,
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
        isDeAMPEnabled: isDeAmpEnabled
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

  func useResources(from info: ResourcesInfo) async throws {
    try await engine.useResources(fromFileURL: info.localFileURL)
    resourcesInfo = info
  }

  /// Serialize the engine into data to be later loaded from cache
  public func serialize() throws -> Data {
    return try engine.serialize()
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
      "\(type.debugDescription) (\(group.fileType.debugDescription))"
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
