/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import WebKit
import Shared
import Data
import BraveCore
import BraveShared

private let log = Logger.browserLogger

class UserScriptManager {

  // Scripts can use this to verify the app –not js on the page– is calling into them.
  private static let securityToken = UUID()

  // Ensures that the message handlers cannot be invoked by the page scripts
  private static let messageHandlerToken = UUID()

  // String representation of messageHandlerToken
  public static let messageHandlerTokenString = UserScriptManager.messageHandlerToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)

  // String representation of securityToken
  public static let securityTokenString = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)

  private weak var tab: Tab?

  /// Whether cookie blocking is enabled
  var isCookieBlockingEnabled: Bool {
    didSet {
      if oldValue == isCookieBlockingEnabled { return }
      reloadUserScripts()
    }
  }

  /// Whether or not the PaymentRequest APIs should be exposed
  var isPaymentRequestEnabled: Bool {
    didSet {
      if oldValue == isPaymentRequestEnabled { return }
      reloadUserScripts()
    }
  }

  /// Whether or not Playlist is enabled
  var isPlaylistEnabled: Bool {
    didSet {
      if oldValue == isPlaylistEnabled { return }
      reloadUserScripts()
    }
  }

  /// Whether or not the MediaSource API should be disabled for Playlists
  var isWebCompatibilityMediaSourceAPIEnabled: Bool {
    didSet {
      if oldValue == isWebCompatibilityMediaSourceAPIEnabled { return }
      reloadUserScripts()
    }
  }

  /// Whether or not the Media Background Playback is enabled
  var isMediaBackgroundPlaybackEnabled: Bool {
    didSet {
      if oldValue == isMediaBackgroundPlaybackEnabled { return }
      reloadUserScripts()
    }
  }

  /// Whether night mode is enabled for webview
  var isNightModeEnabled: Bool {
    didSet {
      if oldValue == isNightModeEnabled { return }
      reloadUserScripts()
    }
  }
  
  /// Whether deamp is enabled for webview
  var isDeAMPEnabled: Bool {
    didSet {
      guard oldValue != isDeAMPEnabled else { return }
      reloadUserScripts()
    }
  }
  
  /// Whether request blocking is enabled for webview
  var isRequestBlockingEnabled: Bool {
    didSet {
      guard oldValue != isRequestBlockingEnabled else { return }
      reloadUserScripts()
    }
  }

  // TODO: @JS Add other scripts to this list to avoid uneccesary calls to `reloadUserScripts()`
  /// Domain script types that are currently injected into the web-view. Will reloaded scripts if this set changes.
  ///
  /// We only `reloadUserScripts()` if any of these have changed. A set is used to ignore order and ensure uniqueness.
  /// This way we don't necessarily invoke `reloadUserScripts()` too often but only when necessary.
  ///
  var userScriptTypes: Set<UserScriptType> {
    didSet {
      guard oldValue != userScriptTypes else { return }
      reloadUserScripts()
    }
  }

  public static func isMessageHandlerTokenMissing(in body: [String: Any]) -> Bool {
    guard let token = body["securitytoken"] as? String, token == UserScriptManager.messageHandlerTokenString else {
      return true
    }
    return false
  }

  init(
    tab: Tab,
    isCookieBlockingEnabled: Bool,
    isPaymentRequestEnabled: Bool,
    isWebCompatibilityMediaSourceAPIEnabled: Bool,
    isMediaBackgroundPlaybackEnabled: Bool,
    isNightModeEnabled: Bool,
    isDeAMPEnabled: Bool,
    walletEthProviderJS: String?
  ) {
    self.tab = tab
    self.isCookieBlockingEnabled = isCookieBlockingEnabled
    self.isPaymentRequestEnabled = isPaymentRequestEnabled
    self.isWebCompatibilityMediaSourceAPIEnabled = isWebCompatibilityMediaSourceAPIEnabled
    self.isPlaylistEnabled = true
    self.isMediaBackgroundPlaybackEnabled = isMediaBackgroundPlaybackEnabled
    self.isNightModeEnabled = isNightModeEnabled
    self.isDeAMPEnabled = isDeAMPEnabled
    self.userScriptTypes = []
    self.walletEthProviderJS = walletEthProviderJS
    self.isRequestBlockingEnabled = true
    
    reloadUserScripts()
  }

  // MARK: -

  private let packedUserScripts: [WKUserScript] = {
    [
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: true),
    ].compactMap { arg in
      let (injectionTime, mainFrameOnly, sandboxed) = arg
      let name = (mainFrameOnly ? "MainFrame" : "AllFrames") + "AtDocument" + (injectionTime == .atDocumentStart ? "Start" : "End") + (sandboxed ? "Sandboxed" : "")
      if let path = Bundle.current.path(forResource: name, ofType: "js"),
        let source = try? NSString(contentsOfFile: path, encoding: String.Encoding.utf8.rawValue) as String {
        let wrappedSource = "(function() { const SECURITY_TOKEN = '\(UserScriptManager.messageHandlerTokenString)'; \(source) })()"

        return WKUserScript.create(
          source: wrappedSource,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page)
      }
      return nil
    }
  }()

  private let cookieControlUserScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "CookieControl", ofType: "js"), let source: String = try? String(contentsOfFile: path) else {
      log.error("Failed to load cookie control user script")
      return nil
    }

    return WKUserScript.create(
      source: source,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .page)
  }()
  
  /// A script that adds blocking for invalid fetch and ajax calls
  private let requestBlockingUserScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "RequestBlocking", ofType: "js"), var source: String = try? String(contentsOfFile: path) else {
      log.error("Failed to load cookie control user script")
      return nil
    }
    
    do {
      guard let fakeParams = try UserScriptHelper.makeRequestBlockingParams(securityToken: securityTokenString) else {
        assertionFailure("A nil here is impossible")
        return nil
      }
      
      source = source.replacingOccurrences(of: "$<request_blocking_args>", with: fakeParams)

      return WKUserScript.create(
        source: source,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: false,
        in: .page
      )
    } catch {
      assertionFailure(error.localizedDescription)
      log.error(error.localizedDescription)
      return nil
    }
  }()
  
  /// A script that detects if we're at an amp page and redirects the user to the original (canonical) version if available.
  ///
  /// - Note: This script is only a smaller part (2 of 3) of de-amping.
  /// The first part is handled by an ad-block rule and enabled via a `deAmpEnabled` boolean in `AdBlockStats`
  /// The third part is handled by debouncing amp links and handled by debouncing logic in `DebouncingResourceDownloader`
  private let deAMPUserScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "DeAMP", ofType: "js"), var source: String = try? String(contentsOfFile: path) else {
      log.error("Failed to load cookie control user script")
      return nil
    }
    
    do {
      guard let arguments = try UserScriptHelper.makeDeAmpScriptParamters() else {
        assertionFailure("A nil here is impossible")
        return nil
      }

      source = source.replacingOccurrences(of: "$<deamp_args>", with: arguments, options: .literal)

      return WKUserScript.create(
        source: source,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: true,
        in: .defaultClient)
    } catch {
      assertionFailure(error.localizedDescription)
      return nil
    }
  }()

  // PaymentRequestUserScript is injected at document start to handle
  // requests to payment APIs
  private let PaymentRequestUserScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "PaymentRequest", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load PaymentRequest.js")
      return nil
    }

    var alteredSource = source

    alteredSource = alteredSource.replacingOccurrences(of: "$<paymentreq>", with: "PaymentRequest\(securityTokenString)", options: .literal)
    alteredSource = alteredSource.replacingOccurrences(of: "$<paymentresponse>", with: "PaymentResponse\(securityTokenString)", options: .literal)
    alteredSource = alteredSource.replacingOccurrences(of: "$<paymentresponsedetails>", with: "PaymentResponseDetails\(securityTokenString)", options: .literal)
    alteredSource = alteredSource.replacingOccurrences(of: "$<paymentreqcallback>", with: "PaymentRequestCallback\(securityTokenString)", options: .literal)
    alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "PaymentRequest\(messageHandlerTokenString)", options: .literal)

    return WKUserScript.create(
      source: alteredSource,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .page)
  }()

  private let resourceDownloadManagerUserScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "ResourceDownloader", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load ResourceDownloader.js")
      return nil
    }
    var alteredSource: String = source

    alteredSource = alteredSource.replacingOccurrences(of: "$<downloadManager>", with: "D\(securityTokenString)", options: .literal)
    alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "ResourceDownloadManager\(messageHandlerTokenString)", options: .literal)

    return WKUserScript.create(
      source: alteredSource,
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: false,
      in: .defaultClient)
  }()

  private let WindowRenderHelperScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "WindowRenderHelper", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load WindowRenderHelper.js")
      return nil
    }

    // Verify that the application itself is making a call to the JS script instead of other scripts on the page.
    // This variable will be unique amongst scripts loaded in the page.
    // When the script is called, the token is provided in order to access teh script variable.
    var alteredSource = source

    alteredSource = alteredSource.replacingOccurrences(of: "$<windowRenderer>", with: "W\(securityTokenString)", options: .literal)
    alteredSource = alteredSource.replacingOccurrences(of: "$<handler>", with: "WindowRenderHelper\(messageHandlerTokenString)", options: .literal)

    return WKUserScript.create(
      source: alteredSource,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .defaultClient)
  }()

  private let FullscreenHelperScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "FullscreenHelper", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load FullscreenHelper.js")
      return nil
    }

    return WKUserScript.create(
      source: source,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .page)
  }()

  private let PlaylistSwizzlerScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "PlaylistSwizzler", ofType: "js"),
      let source = try? String(contentsOfFile: path)
    else {
      log.error("Failed to load PlaylistSwizzler.js")
      return nil
    }

    return WKUserScript.create(
      source: source,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .page)
  }()

  private let PlaylistHelperScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "Playlist", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load Playlist.js")
      return nil
    }

    var alteredSource = source
    let token = UserScriptManager.securityTokenString

    let replacements = [
      "$<Playlist>": "Playlist_\(token)",
      "$<security_token>": "\(token)",
      "$<tagNode>": "tagNode_\(token)",
      "$<tagUUID>": "tagUUID_\(token)",
      "$<mediaCurrentTimeFromTag>": "mediaCurrentTimeFromTag_\(token)",
      "$<stopMediaPlayback>": "stopMediaPlayback_\(token)",
      "$<sendMessage>": "playlistHelper_sendMessage_\(token)",
      "$<handler>": "playlistHelper_\(messageHandlerTokenString)",
      "$<notify>": "notify_\(token)",
      "$<onLongPressActivated>": "onLongPressActivated_\(token)",
      "$<setupLongPress>": "setupLongPress_\(token)",
      "$<setupDetector>": "setupDetector_\(token)",
      "$<setupTagNode>": "setupTagNode_\(token)",
      "$<notifyNodeSource>": "notifyNodeSource_\(token)",
      "$<notifyNode>": "notifyNode_\(token)",
      "$<observeNode>": "observeNode_\(token)",
      "$<observeDocument>": "observeDocument_\(token)",
      "$<observeDynamicElements>": "observeDynamicElements_\(token)",
      "$<getAllVideoElements>": "getAllVideoElements_\(token)",
      "$<getAllAudioElements>": "getAllAudioElements_\(token)",
      "$<onReady>": "onReady_\(token)",
      "$<requestWhenIdleShim>": "requestWhenIdleShim_\(token)",
      "$<observePage>": "observePage_\(token)",
    ]

    replacements.forEach({
      alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
    })

    return WKUserScript.create(
      source: alteredSource,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .page)
  }()

  private let MediaBackgroundingScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "MediaBackgrounding", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load MediaBackgrounding.js")
      return nil
    }

    var alteredSource = source
    let token = UserScriptManager.securityTokenString

    let replacements = [
      "$<MediaBackgrounding>": "MediaBackgrounding_\(token)",
      "$<handler>": "mediaBackgrounding_\(messageHandlerTokenString)",
    ]

    replacements.forEach({
      alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
    })

    return WKUserScript.create(
      source: alteredSource,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: .page)
  }()

  private let NightModeScript: WKUserScript? = {
    return WKUserScript.create(
      source: "window.__firefox__.NightMode.setEnabled(true);",
      injectionTime: .atDocumentStart,
      forMainFrameOnly: true,
      in: .defaultClient)
  }()
  
  private let ReadyStateScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "ReadyState", ofType: "js"), let source = try? String(contentsOfFile: path) else {
      log.error("Failed to load ReadyState.js")
      return nil
    }

    var alteredSource = source
    let token = UserScriptManager.securityTokenString

    let replacements = [
      "$<security_token>": token,
      "$<handler>": "ReadyState_\(messageHandlerTokenString)",
    ]

    replacements.forEach({
      alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
    })

    return WKUserScript.create(
      source: alteredSource,
      injectionTime: .atDocumentStart,
      forMainFrameOnly: true,
      in: .page)
  }()

  private let walletEthProviderScript: WKUserScript? = {
    guard let path = Bundle.current.path(forResource: "WalletEthereumProvider", ofType: "js"),
          let source = try? String(contentsOfFile: path) else {
      return nil
    }
    
    var alteredSource = source
    
    let replacements = [
      "$<security_token>": UserScriptManager.securityTokenString,
      "$<handler>": "walletEthereumProvider_\(messageHandlerTokenString)",
    ]
    
    replacements.forEach({
      alteredSource = alteredSource.replacingOccurrences(of: $0.key, with: $0.value, options: .literal)
    })
    
    return WKUserScript(source: alteredSource,
                        injectionTime: .atDocumentStart,
                        forMainFrameOnly: true,
                        in: .page)
  }()

  private var walletEthProviderJS: String?
    
  private func reloadUserScripts() {
    tab?.webView?.configuration.userContentController.do {
      $0.removeAllUserScripts()
      // This has to be added before `packedUserScripts` because some scripts do
      // rewarding even if the request is blocked.
      if isRequestBlockingEnabled, let script = requestBlockingUserScript {
        $0.addUserScript(script)
      }
      
      self.packedUserScripts.forEach($0.addUserScript)
      
      if isCookieBlockingEnabled, let script = cookieControlUserScript {
        $0.addUserScript(script)
      }

      if let script = resourceDownloadManagerUserScript {
        $0.addUserScript(script)
      }

      if let script = WindowRenderHelperScript {
        $0.addUserScript(script)
      }

      if let script = FullscreenHelperScript {
        $0.addUserScript(script)
      }

      if UIDevice.isIpad, isWebCompatibilityMediaSourceAPIEnabled, let script = PlaylistSwizzlerScript {
        $0.addUserScript(script)
      }

      if isPlaylistEnabled, let script = PlaylistHelperScript {
        $0.addUserScript(script)
      }

      if isMediaBackgroundPlaybackEnabled, let script = MediaBackgroundingScript {
        $0.addUserScript(script)
      }

      if isNightModeEnabled, let script = NightModeScript {
        $0.addUserScript(script)
      }
      
      if isDeAMPEnabled, let script = deAMPUserScript {
        $0.addUserScript(script)
      }
      
      if let script = ReadyStateScript {
        $0.addUserScript(script)
      }

      for userScriptType in userScriptTypes.sorted(by: { $0.order < $1.order }) {
        do {
          let script = try ScriptFactory.shared.makeScript(for: userScriptType)
          $0.addUserScript(script)
        } catch {
          assertionFailure("Should never happen. The scripts are packed in the project and loading/modifying should always be possible.")
          log.error(error)
        }
      }

      if let script = walletEthProviderScript,
         tab?.isPrivate == false,
         Preferences.Wallet.WalletType(rawValue: Preferences.Wallet.defaultEthWallet.value) == .brave {
        $0.addUserScript(script)
        if var providerJS = walletEthProviderJS {
          providerJS = """
            (function() {
              if (window.isSecureContext) {
                \(providerJS)
              }
            })();
            """
          $0.addUserScript(.init(source: providerJS, injectionTime: .atDocumentStart, forMainFrameOnly: true, in: .page))
        }
      }
    }
  }
}
