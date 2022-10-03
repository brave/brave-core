// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
  private var cachedScriptSources: [ScriptSourceType: String]
  
  /// This contains cached altered scripts that are ready to be inected. Avoids replacing strings.
  private var cachedDomainScriptsSources: [UserScriptType: WKUserScript]
  
  init() {
    cachedScriptSources = [:]
    cachedDomainScriptsSources = [:]
  }
  
  /// Clear some caches in case we need to.
  ///
  /// Should only really be called in a memory warning scenario.
  func clearCaches() {
    cachedScriptSources = [:]
    cachedDomainScriptsSources = [:]
  }
  
  /// Returns a script source by loading a file or returning cached data
  private func makeScriptSource(of type: ScriptSourceType) throws -> String {
    if let source = cachedScriptSources[type] {
      return source
    } else {
      let source = try type.loadScript()
      cachedScriptSources[type] = source
      return source
    }
  }
  
  /// Get a script for the `UserScriptType`.
  ///
  /// Scripts can be cached on two levels:
  /// - On the unmodified source file (per `ScriptSourceType`)
  /// - On the modfied source file (per `UserScriptType`)
  func makeScript(for domainType: UserScriptType) throws -> WKUserScript {
    // First check for and return cached value
    if let script = cachedDomainScriptsSources[domainType] {
      return script
    }
    
    var source = try makeScriptSource(of: domainType.sourceType)
    
    switch domainType {
    case .siteStateListener:
      guard let script = SiteStateListenerScriptHandler.userScript else {
        assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
        throw ScriptLoadFailure.notFound
      }
      cachedDomainScriptsSources[domainType] = script
      return script
      
    case .farblingProtection(let etld):
      let randomConfiguration = RandomConfiguration(etld: etld)
      let fakeParams = try FarblingProtectionHelper.makeFarblingParams(from: randomConfiguration)
      source = source.replacingOccurrences(of: "$<farbling_protection_args>", with: fakeParams)
      
    case .nacl:
      // No modifications needed
      break
      
    case .domainUserScript(let domainUserScript):
      switch domainUserScript {
      case .braveSearchHelper:
        guard let script = BraveSearchScriptHandler.userScript else {
          assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
          throw ScriptLoadFailure.notFound
        }
        cachedDomainScriptsSources[domainType] = script
        return script
        
      case .braveTalkHelper:
        guard let script = BraveTalkScriptHandler.userScript else {
          assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
          throw ScriptLoadFailure.notFound
        }
        cachedDomainScriptsSources[domainType] = script
        return script
        
      case .braveSkus:
        guard let script = BraveSkusScriptHandler.userScript else {
          assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
          throw ScriptLoadFailure.notFound
        }
        cachedDomainScriptsSources[domainType] = script
        return script
        
      case .bravePlaylistFolderSharingHelper:
        guard let script = PlaylistFolderSharingScriptHandler.userScript else {
          assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
          throw ScriptLoadFailure.notFound
        }
        cachedDomainScriptsSources[domainType] = script
        return script
        
      case .youtubeAdblock:
        // No modifications needed
        break
      }
    }
    
    let userScript = WKUserScript.create(source: source, injectionTime: domainType.injectionTime, forMainFrameOnly: domainType.forMainFrameOnly, in: domainType.contentWorld)
    cachedDomainScriptsSources[domainType] = userScript
    return userScript
  }
}
