/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Combine
import BraveCore

private let log = Logger.browserLogger

struct CosmeticFilterModel: Codable {
  let hideSelectors: [String]
  let styleSelectors: [String: [String]]
  let exceptions: [String]
  let injectedScript: String
  let genericHide: Bool
  
  enum CodingKeys: String, CodingKey {
    case hideSelectors = "hide_selectors"
    case styleSelectors = "style_selectors"
    case exceptions = "exceptions"
    case injectedScript = "injected_script"
    case genericHide = "generichide"
  }
  
  func makeCSSRules() -> String {
    let hideRules = hideSelectors.reduce("") { partialResult, rule in
      return [partialResult, rule, "{display: none !important}\n"].joined()
    }
    
    let styleRules = styleSelectors.reduce("") { partialResult, entry in
      let subRules = entry.value.reduce("") { partialResult, subRule in
        return [partialResult, subRule, ";"].joined()
      }
      
      return [partialResult, entry.key, "{", subRules, " !important}\n"].joined()
    }
    
    return [hideRules, styleRules].joined()
  }
}

public class AdBlockStats {
  public static let shared = AdBlockStats()

  fileprivate var fifoCacheOfUrlsChecked = FifoDict<Bool>()

  // Adblock engine for general adblock lists.
  private(set) var engines: [AdblockEngine]

  /// The task that downloads all the files. Can be cancelled
  private var downloadTask: AnyCancellable?

  init() {
    engines = []
  }

  static let adblockSerialQueue = DispatchQueue(label: "com.brave.adblock-dispatch-queue")
  
  func clearCaches() {
    fifoCacheOfUrlsChecked = FifoDict<Bool>()
  }
  
  /// Checks the general and regional engines to see if the request should be blocked.
  ///
  /// - Note: This method is should not be synced on `AdBlockStatus.adblockSerialQueue` and the result is synced on the main thread.
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType, callback: @escaping (Bool) -> Void) {
    Self.adblockSerialQueue.async { [weak self] in
      let shouldBlock = self?.shouldBlock(requestURL: requestURL, sourceURL: sourceURL, resourceType: resourceType) == true
      
      DispatchQueue.main.async {
        callback(shouldBlock)
      }
    }
  }
  
  /// Checks the general and regional engines to see if the request should be blocked
  ///
  /// - Warning: This method needs to be synced on `AdBlockStatus.adblockSerialQueue`
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType) -> Bool {
    let key = [requestURL.absoluteString, sourceURL.absoluteString, resourceType.rawValue].joined(separator: "_")
    
    if let cachedResult = fifoCacheOfUrlsChecked.getElement(key) {
        return cachedResult
    }
    
    let shouldBlock = engines.contains(where: { engine in
      return engine.shouldBlock(
        requestURL: requestURL,
        sourceURL: sourceURL,
        resourceType: resourceType
      )
    })
    
    fifoCacheOfUrlsChecked.addElement(shouldBlock, forKey: key)
    return shouldBlock
  }
  
  func set(engines: [AdblockEngine]) {
    self.engines = engines
    self.clearCaches()
  }
  
  func makeEngineScriptSouces(for url: URL) throws -> [String] {
    return try engines.flatMap { engine -> [String] in
      var results: [String] = []
      let sources = try engine.makeEngineScriptSources(for: url)
      
      if let source = sources.cssInjectScript {
        results.append(source)
      }
      
      if let source = sources.generalScript {
        results.append(source)
      }
      
      return results
    }
  }
}

extension AdblockEngine {
  func makeEngineScriptSources(for url: URL) throws -> (cssInjectScript: String?, generalScript: String?) {
    let rules = cosmeticResourcesForURL(url.absoluteString)
    guard let data = rules.data(using: .utf8) else { return (nil, nil) }
    let model = try JSONDecoder().decode(CosmeticFilterModel.self, from: data)
    let cssRules = model.makeCSSRules()
    let cssInjectScript: String?
    let generalScript: String?
    
    if !cssRules.isEmpty {
      cssInjectScript = """
      (function() {
        var head = document.head || document.getElementsByTagName('head')[0];
        if (head == null) {
          return;
        }
        
        var style = document.createElement('style');
        style.type = 'text/css';
      
        var styles = atob("\(cssRules.toBase64())");
        
        if (style.styleSheet) {
          style.styleSheet.cssText = styles;
        } else {
          style.appendChild(document.createTextNode(styles));
        }

        head.appendChild(style);
      })();
      """
    } else {
      cssInjectScript = nil
    }
    
    if !model.injectedScript.isEmpty {
      var injectedScriptSource = model.injectedScript
      
      injectedScriptSource = [
        "(function(){",
        /// This boolean is used by a script injected by cosmetic filters and enables that script via this boolean
        /// The script is found here: https://github.com/brave/adblock-resources/blob/master/resources/de-amp.js
        /// - Note: This script is only a smaller part (1 of 3) of de-amping:
        /// The second part is handled by an inected script that redirects amp pages to their canonical links
        /// The third part is handled by debouncing amp links and handled by debouncing rules
        Preferences.Shields.autoRedirectAMPPages.value ? "const deAmpEnabled = true;" : "",
        injectedScriptSource,
        "})();"
      ].joined(separator: "\n")
      
      generalScript = injectedScriptSource
    } else {
      generalScript = nil
    }
    
    return (cssInjectScript, generalScript)
  }
}
