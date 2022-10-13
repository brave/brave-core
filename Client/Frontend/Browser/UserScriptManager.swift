/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
import BraveCore
import BraveShared
import os.log

private class ScriptLoader: TabContentScriptLoader { }

class UserScriptManager {
  static let shared = UserScriptManager()
  
  static let securityToken = ScriptLoader.uniqueID
  
  private let alwaysEnabledScripts: [ScriptType] = [
    .rewardsReporting,
    .playlist,
    .resourceDownloader,
    .windowRenderHelper,
    .readyStateHelper
  ]
  
  /// Scripts that are loaded after `staticScripts`
  private let dynamicScripts: [ScriptType: WKUserScript] = {
    ScriptType.allCases.reduce(into: [:]) { $0[$1] = $1.script }
  }()
  
  /// Scripts that are web-packed and should be loaded after `baseScripts` but before `dynamicScripts`
  private let staticScripts: [WKUserScript] = {
    return [
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: true),
    ].compactMap { (injectionTime, mainFrameOnly, sandboxed) in
      
      let name = (mainFrameOnly ? "MainFrame" : "AllFrames") + "AtDocument" + (injectionTime == .atDocumentStart ? "Start" : "End") + (sandboxed ? "Sandboxed" : "")
      
      if let source = ScriptLoader.loadUserScript(named: name) {
        let wrappedSource = "(function() { const SECURITY_TOKEN = '\(UserScriptManager.securityToken)'; \(source) })()"

        return WKUserScript.create(
          source: wrappedSource,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page)
      }
      
      return nil
    }
  }()
  
  /// Scripts injected before all other scripts.
  private let baseScripts: [WKUserScript] = {
    [
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: true),
    ].compactMap { (injectionTime, mainFrameOnly, sandboxed) in
      
      if let source = ScriptLoader.loadUserScript(named: "__firefox__") {
        return WKUserScript.create(
          source: source,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page)
      }
      
      return nil
    }
  }()
  
  enum ScriptType: String, CaseIterable {
    case cookieBlocking
    case rewardsReporting
    case mediaBackgroundPlay
    case playlistMediaSource
    case playlist
    case nightMode
    case deAmp
    case requestBlocking
    case resourceDownloader
    case windowRenderHelper
    case readyStateHelper
    case ethereumProvider
    
    fileprivate var script: WKUserScript? {
      switch self {
        // Conditionally enabled scripts
      case .cookieBlocking: return loadScript(named: "CookieControlScript")
      case .mediaBackgroundPlay: return loadScript(named: "MediaBackgroundingScript")
      case .playlistMediaSource: return loadScript(named: "PlaylistSwizzlerScript")
      case .nightMode: return NightModeScriptHandler.userScript
      case .deAmp: return DeAmpScriptHandler.userScript
      case .requestBlocking: return RequestBlockingContentScriptHandler.userScript
      case .ethereumProvider: return EthereumProviderScriptHandler.userScript
        
      // Always enabled scripts
      case .rewardsReporting: return RewardsReportingScriptHandler.userScript
      case .playlist: return PlaylistScriptHandler.userScript
      case .resourceDownloader: return ResourceDownloadScriptHandler.userScript
      case .windowRenderHelper: return WindowRenderScriptHandler.userScript
      case .readyStateHelper: return ReadyStateScriptHandler.userScript
      }
    }
    
    private func loadScript(named: String) -> WKUserScript? {
      guard var script = ScriptLoader.loadUserScript(named: named) else {
        return nil
      }
      
      script = ScriptLoader.secureScript(handlerNamesMap: [:], securityToken: "", script: script)
      return WKUserScript.create(source: script, injectionTime: .atDocumentStart, forMainFrameOnly: false, in: .page)
    }
  }
  
  private func loadScripts(into tab: Tab, scripts: Set<ScriptType>) {
    guard let webView = tab.webView else {
      assertionFailure("Injecting Scripts into a Tab that has no WebView")
      return
    }
    
    var scripts = scripts
    
    webView.configuration.userContentController.do { scriptController in
      scriptController.removeAllUserScripts()
      
      // Inject all base scripts
      self.baseScripts.forEach {
        scriptController.addUserScript($0)
      }
      
      // Inject specifically RequestBlocking BEFORE other scripts
      // this is because it needs to hook requests before RewardsReporting
      if scripts.contains(.requestBlocking), let script = self.dynamicScripts[.requestBlocking] {
        scripts.remove(.requestBlocking)
        scriptController.addUserScript(script)
      }
      
      // Inject all static scripts
      self.staticScripts.forEach {
        scriptController.addUserScript($0)
      }
      
      // Inject all scripts that are dynamic, but always enabled
      self.dynamicScripts.filter({ self.alwaysEnabledScripts.contains($0.key) }).forEach {
        scriptController.addUserScript($0.value)
      }
      
      // Inject all optional scripts
      self.dynamicScripts.filter({ scripts.contains($0.key) }).forEach {
        scriptController.addUserScript($0.value)
      }
    }
  }
  
  // TODO: Get rid of this OR refactor wallet and domain scripts
  func loadCustomScripts(into tab: Tab, userScripts: Set<ScriptType>, customScripts: Set<UserScriptType>, walletEthProviderScript: WKUserScript?) {
    guard let webView = tab.webView else {
      assertionFailure("Injecting Scripts into a Tab that has no WebView")
      return
    }
    
    loadScripts(into: tab, scripts: userScripts)
    
    webView.configuration.userContentController.do { scriptController in
      // TODO: Somehow refactor wallet and get rid of this
      // Inject WALLET specific scripts
      
      if tab.isPrivate == false,
         Preferences.Wallet.WalletType(rawValue: Preferences.Wallet.defaultEthWallet.value) == .brave,
         let script = self.dynamicScripts[.ethereumProvider] {
        
        // Inject ethereum provider
        scriptController.addUserScript(script)
        
        if let walletEthProviderScript = walletEthProviderScript {
          scriptController.addUserScript(walletEthProviderScript)
        }
      }
      
      // TODO: Refactor this and get rid of the `UserScriptType`
      // Inject Custom scripts
      for userScriptType in customScripts.sorted(by: { $0.order < $1.order }) {
        do {
          let script = try ScriptFactory.shared.makeScript(for: userScriptType)
          scriptController.addUserScript(script)
        } catch {
          assertionFailure("Should never happen. The scripts are packed in the project and loading/modifying should always be possible.")
          Logger.module.error("\(error.localizedDescription)")
        }
      }
    }
  }
}
