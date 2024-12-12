// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveWallet
import Data
import Preferences
import Shared
import WebKit
import os.log

private class ScriptLoader: TabContentScriptLoader {}

class UserScriptManager {
  static let shared = UserScriptManager()

  static let securityToken = ScriptLoader.uniqueID
  static let walletSolanaNameSpace = "W\(ScriptLoader.uniqueID)"

  private var alwaysEnabledScripts: [ScriptType] {
    var scripts: [ScriptType] = [
      .faviconFetcher,
      .rewardsReporting,
      .resourceDownloader,
      .nightMode,
    ]

    if Preferences.UserScript.playlist.value {
      scripts.append(.playlist)
    }

    if Preferences.UserScript.readyState.value {
      scripts.append(.readyStateHelper)
    }

    if Preferences.UserScript.youtubeQuality.value {
      scripts.append(.youtubeQuality)
    }

    if Preferences.UserScript.leo.value {
      scripts.append(.braveLeoAIChat)
    }

    return scripts
  }

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

      let name =
        (mainFrameOnly ? "MainFrame" : "AllFrames") + "AtDocument"
        + (injectionTime == .atDocumentStart ? "Start" : "End") + (sandboxed ? "Sandboxed" : "")

      if let source = ScriptLoader.loadUserScript(named: name) {
        let wrappedSource =
          "(function() { const SECURITY_TOKEN = '\(UserScriptManager.securityToken)'; \(source) })()"

        return WKUserScript(
          source: wrappedSource,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page
        )
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
        return WKUserScript(
          source: source,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page
        )
      }

      return nil
    }
  }()

  private var walletEthProviderScript: WKUserScript?
  private var walletSolProviderScript: WKUserScript?
  private var walletSolanaWeb3Script: WKUserScript?
  private var walletSolanaWalletStandardScript: WKUserScript?

  enum ScriptType: String, CaseIterable {
    case faviconFetcher
    case cookieBlocking
    case rewardsReporting
    case mediaBackgroundPlay
    case playlistMediaSource
    case playlist
    case nightMode
    case deAmp
    case requestBlocking
    case trackerProtectionStats
    case resourceDownloader
    case readyStateHelper
    case ethereumProvider
    case solanaProvider
    case searchResultAd
    case youtubeQuality
    case braveLeoAIChat

    fileprivate var script: WKUserScript? {
      switch self {
      // Conditionally enabled scripts
      case .cookieBlocking:
        return Preferences.UserScript.cookieBlocking.value
          ? loadScript(named: "CookieControlScript") : nil
      case .mediaBackgroundPlay:
        return Preferences.UserScript.mediaBackgroundPlay.value
          ? loadScript(named: "MediaBackgroundingScript") : nil
      case .playlistMediaSource:
        return Preferences.UserScript.mediaSource.value
          ? loadScript(named: "PlaylistSwizzlerScript") : nil
      case .deAmp: return Preferences.UserScript.deAmp.value ? DeAmpScriptHandler.userScript : nil
      case .requestBlocking:
        return Preferences.UserScript.requestBlocking.value
          ? RequestBlockingContentScriptHandler.userScript : nil
      case .trackerProtectionStats:
        return Preferences.UserScript.trackingProtectionStats.value
          ? ContentBlockerHelper.userScript : nil
      case .ethereumProvider:
        return Preferences.UserScript.ethereumProvider.value
          ? EthereumProviderScriptHandler.userScript : nil
      case .solanaProvider:
        return Preferences.UserScript.solanaProvider.value
          ? SolanaProviderScriptHandler.userScript : nil
      case .searchResultAd: return BraveSearchResultAdScriptHandler.userScript

      // Always enabled scripts
      case .faviconFetcher: return FaviconScriptHandler.userScript
      case .nightMode: return DarkReaderScriptHandler.userScript
      case .rewardsReporting:
        return Preferences.UserScript.rewardsReporting.value
          ? RewardsReportingScriptHandler.userScript : nil
      case .playlist:
        return Preferences.UserScript.playlist.value ? PlaylistScriptHandler.userScript : nil
      case .resourceDownloader: return ResourceDownloadScriptHandler.userScript
      case .readyStateHelper:
        return Preferences.UserScript.readyState.value ? ReadyStateScriptHandler.userScript : nil
      case .youtubeQuality:
        return Preferences.UserScript.youtubeQuality.value
          ? YoutubeQualityScriptHandler.userScript : nil
      case .braveLeoAIChat:
        return Preferences.UserScript.leo.value ? BraveLeoScriptHandler.userScript : nil
      }
    }

    private func loadScript(named: String) -> WKUserScript? {
      guard var script = ScriptLoader.loadUserScript(named: named) else {
        return nil
      }

      script = ScriptLoader.secureScript(handlerNamesMap: [:], securityToken: "", script: script)
      return WKUserScript(
        source: script,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: false,
        in: .page
      )
    }
  }

  func fetchWalletScripts(from braveWalletAPI: BraveWalletAPI) {
    if let ethJS = braveWalletAPI.providerScripts(for: .eth)[.ethereum] {
      let providerJS = """
        window.__firefox__.execute(function($, $Object) {
          if (window.isSecureContext) {
            \(ethJS)
          }
        });
        """
      walletEthProviderScript = WKUserScript(
        source: providerJS,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: true,
        in: EthereumProviderScriptHandler.scriptSandbox
      )
    }
    if let solanaWeb3Script = braveWalletAPI.providerScripts(for: .sol)[.solanaWeb3] {
      let script = """
        // Define a global variable with a random name
        // Local variables are NOT enumerable!
        let \(UserScriptManager.walletSolanaNameSpace);

        window.__firefox__.execute(function($, $Object, $Function, $Array) {
          // Inject Solana as a Local Variable.
          \(solanaWeb3Script)

          \(UserScriptManager.walletSolanaNameSpace) = $({
            solanaWeb3: $(solanaWeb3)
          });

          // Failed to load SolanaWeb3
          if (typeof \(UserScriptManager.walletSolanaNameSpace) === 'undefined') {
            return;
          }

          const freezeExceptions = $Array.of("BN");

          for (const value of $Object.values(\(UserScriptManager.walletSolanaNameSpace).solanaWeb3)) {
            if (!value) {
              continue;
            }

            $.extensiveFreeze(value, freezeExceptions);
          }

          $.deepFreeze(\(UserScriptManager.walletSolanaNameSpace).solanaWeb3);
          $.deepFreeze(\(UserScriptManager.walletSolanaNameSpace));
        });
        """
      self.walletSolanaWeb3Script = WKUserScript(
        source: script,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: true,
        in: SolanaProviderScriptHandler.scriptSandbox
      )
    }
    if let walletSolProviderScript = braveWalletAPI.providerScripts(for: .sol)[.solana] {
      let script = """
        window.__firefox__.execute(function($, $Object) {
          \(walletSolProviderScript)
        });
        """
      self.walletSolProviderScript = WKUserScript(
        source: script,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: true,
        in: SolanaProviderScriptHandler.scriptSandbox
      )
    }
    if let walletStandardScript = braveWalletAPI.providerScripts(for: .sol)[.walletStandard] {
      let script = """
        window.__firefox__.execute(function($, $Object) {
           \(walletStandardScript)
           window.addEventListener('wallet-standard:app-ready', (e) => {
              walletStandardBrave.initialize(window.braveSolana);
          })
        });
        """
      self.walletSolanaWalletStandardScript = WKUserScript(
        source: script,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: true,
        in: SolanaProviderScriptHandler.scriptSandbox
      )
    }
  }

  public func loadScripts(into webView: WKWebView, scripts: Set<ScriptType>) {
    if Preferences.UserScript.blockAllScripts.value {
      return
    }

    var scripts = scripts

    webView.configuration.userContentController.do { scriptController in
      scriptController.removeAllUserScripts()

      // Inject all base scripts
      self.baseScripts.forEach {
        scriptController.addUserScript($0)
      }

      // Inject specifically trackerProtectionStats BEFORE request blocking
      // this is because it needs to hook requests before requestBlocking
      if scripts.contains(.trackerProtectionStats),
        let script = self.dynamicScripts[.trackerProtectionStats]
      {
        scripts.remove(.trackerProtectionStats)
        scriptController.addUserScript(script)
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
  func loadCustomScripts(
    into tab: Tab,
    userScripts: Set<ScriptType>,
    customScripts: Set<UserScriptType>
  ) {
    if Preferences.UserScript.blockAllScripts.value {
      return
    }

    guard let webView = tab.webView else {
      Logger.module.info("Injecting Scripts into a Tab that has no WebView")
      return
    }

    let logComponents = [
      userScripts.sorted(by: { $0.rawValue < $1.rawValue }).map { scriptType in
        " \(scriptType.rawValue)"
      }.joined(separator: "\n"),
      customScripts.sorted(by: { $0.order < $1.order }).map { scriptType in
        " #\(scriptType.order) \(scriptType.debugDescription)"
      }.joined(separator: "\n"),
    ]
    ContentBlockerManager.log.debug(
      "Loaded \(userScripts.count + customScripts.count) script(s): \n\(logComponents.joined(separator: "\n"))"
    )
    loadScripts(into: webView, scripts: userScripts)

    webView.configuration.userContentController.do { scriptController in
      // TODO: Somehow refactor wallet and get rid of this
      // Inject WALLET specific scripts

      if !tab.isPrivate,
        Preferences.Wallet.WalletType(rawValue: Preferences.Wallet.defaultEthWallet.value)
          == .brave,
        let script = self.dynamicScripts[.ethereumProvider]
      {

        // Inject ethereum provider
        scriptController.addUserScript(script)

        if let walletEthProviderScript = walletEthProviderScript {
          scriptController.addUserScript(walletEthProviderScript)
        }
      }

      // Inject SolanaWeb3Script.js
      if !tab.isPrivate,
        Preferences.Wallet.WalletType(rawValue: Preferences.Wallet.defaultSolWallet.value)
          == .brave,
        let solanaWeb3Script = Preferences.UserScript.solanaProvider.value
          ? self.walletSolanaWeb3Script : nil
      {
        scriptController.addUserScript(solanaWeb3Script)
      }

      if !tab.isPrivate,
        Preferences.Wallet.WalletType(rawValue: Preferences.Wallet.defaultSolWallet.value)
          == .brave,
        let script = self.dynamicScripts[.solanaProvider]
      {

        // Inject solana provider
        scriptController.addUserScript(script)

        if let walletSolProviderScript = walletSolProviderScript {
          scriptController.addUserScript(walletSolProviderScript)
        }
      }

      if !tab.isPrivate,
        let walletStandardScript = Preferences.UserScript.solanaProvider.value
          ? self.walletSolanaWalletStandardScript : nil
      {
        scriptController.addUserScript(walletStandardScript)
      }

      // TODO: Refactor this and get rid of the `UserScriptType`
      // Inject Custom scripts
      for userScriptType in customScripts.sorted(by: { $0.order < $1.order }) {
        do {
          let script = try ScriptFactory.shared.makeScript(for: userScriptType)
          scriptController.addUserScript(script)
        } catch {
          assertionFailure(
            "Should never happen. The scripts are packed in the project and loading/modifying should always be possible."
          )
          Logger.module.error("\(error.localizedDescription)")
        }
      }
    }
  }
}
