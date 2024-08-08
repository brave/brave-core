// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

/// An error representing failures in loading scripts
enum ScriptLoadFailure: Error {
  case notFound
}

/// A class that helps in the aid of creating and caching scripts.
class ScriptFactory {
  /// A shared instance to be shared throughout the app.
  ///
  /// - Note: Perhaps we will move this to the tab to be managed on a tab basis.
  static let shared = ScriptFactory()

  /// Ensures that the message handlers cannot be invoked by the page scripts
  public static let messageHandlerTokenString: String = {
    return UUID().uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
  }()

  /// This contains cahced script sources for a script type. Avoids reading from disk.
  private var cachedScriptSources: FifoDict<ScriptSourceType, String>

  /// This contains cached altered scripts that are ready to be inected. Avoids replacing strings.
  private var cachedDomainScriptsSources: FifoDict<UserScriptType, WKUserScript>

  init() {
    cachedScriptSources = FifoDict()
    cachedDomainScriptsSources = FifoDict()
  }

  /// Clear some caches in case we need to.
  ///
  /// Should only really be called in a memory warning scenario.
  func clearCaches() {
    cachedScriptSources = FifoDict()
    cachedDomainScriptsSources = FifoDict()
  }

  /// Returns a script source by loading a file or returning cached data
  func makeScriptSource(of type: ScriptSourceType) throws -> String {
    if let source = cachedScriptSources.getElement(type) {
      return source
    } else {
      let source = try type.loadScript()
      cachedScriptSources.addElement(source, forKey: type)
      return source
    }
  }

  /// Create a script for the given domain user script
  private func makeScript(for domainUserScript: DomainUserScript) throws -> WKUserScript {
    switch domainUserScript {
    case .braveSearchHelper:
      guard let script = BraveSearchScriptHandler.userScript else {
        assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
        throw ScriptLoadFailure.notFound
      }

      return script
    #if canImport(BraveTalk)
    case .braveTalkHelper:
      guard let script = BraveTalkScriptHandler.userScript else {
        assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
        throw ScriptLoadFailure.notFound
      }

      return script
    #endif

    case .braveSkus:
      guard let script = BraveSkusScriptHandler.userScript else {
        assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
        throw ScriptLoadFailure.notFound
      }

      return script

    case .bravePlaylistFolderSharingHelper:
      guard let script = PlaylistFolderSharingScriptHandler.userScript else {
        assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
        throw ScriptLoadFailure.notFound
      }

      return script
    }
  }

  /// Get a script for the `UserScriptType`.
  ///
  /// Scripts can be cached on two levels:
  /// - On the unmodified source file (per `ScriptSourceType`)
  /// - On the modfied source file (per `UserScriptType`)
  func makeScript(for domainType: UserScriptType) throws -> WKUserScript {
    // First check for and return cached value
    if let script = cachedDomainScriptsSources.getElement(domainType) {
      return script
    }

    let resultingScript: WKUserScript

    switch domainType {
    case .siteStateListener:
      guard let script = SiteStateListenerScriptHandler.userScript else {
        assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
        throw ScriptLoadFailure.notFound
      }

      resultingScript = script

    case .farblingProtection(let etld):
      var source = try makeScriptSource(of: .farblingProtection)
      let randomConfiguration = RandomConfiguration(etld: etld)
      let fakeParams = try FarblingProtectionHelper.makeFarblingParams(from: randomConfiguration)
      source = source.replacingOccurrences(of: "$<farbling_protection_args>", with: fakeParams)
      resultingScript = WKUserScript(
        source: source,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: false,
        in: .page
      )

    case .nacl:
      let source = try makeScriptSource(of: .nacl)
      resultingScript = WKUserScript(
        source: source,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: false,
        in: .page
      )

    case .gpc(let isEnabled):
      let source = try makeScriptSource(of: .gpc)
        .replacingOccurrences(of: "$<is_enabled>", with: isEnabled ? "true" : "false")
      resultingScript = WKUserScript(
        source: source,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: false,
        in: .page
      )

    case .domainUserScript(let domainUserScript):
      resultingScript = try self.makeScript(for: domainUserScript)

    case .selectorsPoller(let setup, let proceduralActions):
      let encoder = JSONEncoder()
      let data = try encoder.encode(setup)
      let args = String(data: data, encoding: .utf8)!
      let proceduralActionFilters =
        proceduralActions.isEmpty ? "undefined" : "[ \(proceduralActions.joined(separator: ","))]"
      let proceduralFiltersScript = try makeScriptSource(of: .proceduralFilters)
      let source = try ScriptFactory.shared.makeScriptSource(of: .selectorsPoller)
        .replacingOccurrences(of: "$<args>", with: args)
        .replacingOccurrences(of: "$<procedural_filters_script>", with: proceduralFiltersScript)
        .replacingOccurrences(of: "$<procedural_filters>", with: proceduralActionFilters)

      let secureSource = CosmeticFiltersScriptHandler.secureScript(
        handlerNamesMap: [
          "$<message_handler>": CosmeticFiltersScriptHandler.messageHandlerName,
          "$<partiness_message_handler>": URLPartinessScriptHandler.messageHandlerName,
        ],
        securityToken: CosmeticFiltersScriptHandler.scriptId,
        script: source
      )

      resultingScript = WKUserScript(
        source: secureSource,
        injectionTime: .atDocumentEnd,
        forMainFrameOnly: false,
        in: CosmeticFiltersScriptHandler.scriptSandbox
      )

    case .engineScript(let configuration):
      let source = [
        "(function(){",
        // This map is used by some of uBlock Origin's resources
        "const scriptletGlobals = (() => {\nconst forwardedMapMethods = [\"has\", \"get\", \"set\"];\nconst handler = {\nget(target, prop) { if (forwardedMapMethods.includes(prop)) { return Map.prototype[prop].bind(target) } return target.get(prop); },\nset(target, prop, value) { if (!forwardedMapMethods.includes(prop)) { target.set(prop, value); } }\n};\nreturn new Proxy(new Map(), handler);\n})();",
        // This boolean is used by a script injected by cosmetic filters and enables that script via this boolean
        // The script is found here: https://github.com/brave/adblock-resources/blob/master/resources/de-amp.js
        // - Note: This script is only a smaller part (1 of 3) of de-amping:
        // The second part is handled by an inected script that redirects amp pages to their canonical links
        // The third part is handled by debouncing amp links and handled by debouncing rules
        configuration.isDeAMPEnabled ? "const deAmpEnabled = true;" : "",
        configuration.source,
        "})();",
      ].joined(separator: "\n")

      if configuration.isMainFrame {
        resultingScript = WKUserScript(
          source: source,
          injectionTime: .atDocumentStart,
          forMainFrameOnly: true,
          in: .page
        )
      } else {
        var sourceWrapper = try makeScriptSource(of: .frameCheckWrapper)
        sourceWrapper = sourceWrapper.replacingOccurrences(of: "$<scriplet>", with: source)
        sourceWrapper = sourceWrapper.replacingOccurrences(
          of: "$<required_href>",
          with: configuration.frameURL.absoluteString
        )
        resultingScript = WKUserScript(
          source: sourceWrapper,
          injectionTime: .atDocumentStart,
          forMainFrameOnly: false,
          in: .page
        )
      }
    }

    cachedDomainScriptsSources.addElement(resultingScript, forKey: domainType)
    return resultingScript
  }
}
