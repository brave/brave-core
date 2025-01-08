// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import Storage
import SwiftyJSON
import WebKit
import os.log

class LoginsScriptHandler: TabContentScript {
  private let profile: Profile
  private let passwordAPI: BravePasswordAPI

  private var snackBar: SnackBar?

  required init(profile: Profile, passwordAPI: BravePasswordAPI) {
    self.profile = profile
    self.passwordAPI = passwordAPI
  }

  static let scriptName = "LoginsScript"
  static let scriptId = UUID().uuidString
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let messageHandlerName = "loginsScriptMessageHandler"
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }
    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let body = message.body as? [String: AnyObject] else {
      return
    }

    guard let res = body["data"] as? [String: AnyObject] else { return }
    guard let type = res["type"] as? String else { return }

    // Check to see that we're in the foreground before trying to check the logins. We want to
    // make sure we don't try accessing the logins database while we're backgrounded to avoid
    // the system from terminating our app due to background disk access.
    //
    // See https://bugzilla.mozilla.org/show_bug.cgi?id=1307822 for details.
    guard UIApplication.shared.applicationState == .active && !profile.isShutdown else {
      return
    }

    // We don't use the WKWebView's URL since the page can spoof the URL by using document.location
    // right before requesting login data. See bug 1194567 for more context.
    if let url = message.frameInfo.request.url {
      // Since responses go to the main frame, make sure we only listen for main frame requests
      // to avoid XSS attacks.
      if type == "request" {
        passwordAPI.getSavedLogins(for: url, formScheme: .typeHtml) { [weak self] logins in
          guard let self = self else { return }

          if let requestId = res["requestId"] as? String {
            self.autoFillRequestedCredentials(
              formSubmitURL: res["formSubmitURL"] as? String ?? "",
              logins: logins,
              requestId: requestId,
              frameInfo: message.frameInfo,
              tab: tab
            )
          }
        }
      } else if type == "submit" {
        if Preferences.General.saveLogins.value {
          updateORSaveCredentials(for: url, script: res, tab: tab)
        }
      }
    }
  }

  private func updateORSaveCredentials(for url: URL, script: [String: Any], tab: Tab) {
    guard let scriptCredentials = passwordAPI.fetchFromScript(url, script: script),
      let username = scriptCredentials.usernameValue,
      scriptCredentials.usernameElement != nil,
      let password = scriptCredentials.passwordValue,
      scriptCredentials.passwordElement != nil
    else {
      Logger.module.debug("Missing Credentials from script")
      return
    }

    if password.isEmpty {
      Logger.module.debug("Empty Password")
      return
    }

    passwordAPI.getSavedLogins(for: url, formScheme: .typeHtml) { [weak self] logins in
      guard let self = self else { return }

      for login in logins {
        guard let usernameLogin = login.usernameValue else {
          continue
        }

        if usernameLogin.caseInsensitivelyEqual(to: username) {
          if password == login.passwordValue {
            return
          }

          self.showUpdatePrompt(from: login, to: scriptCredentials, tab: tab)
          return
        } else {
          self.showAddPrompt(for: scriptCredentials, tab: tab)
          return
        }
      }

      self.showAddPrompt(for: scriptCredentials, tab: tab)
    }
  }

  private func showAddPrompt(for login: PasswordForm, tab: Tab) {
    addSnackBarForPrompt(for: login, tab: tab, isUpdating: false) { [weak self] in
      guard let self = self else { return }

      DispatchQueue.main.async {
        self.passwordAPI.addLogin(login)
      }
    }
  }

  private func showUpdatePrompt(from old: PasswordForm, to new: PasswordForm, tab: Tab) {
    addSnackBarForPrompt(for: new, tab: tab, isUpdating: true) { [weak self] in
      guard let self = self else { return }

      self.passwordAPI.updateLogin(new, oldPasswordForm: old)
    }
  }

  private func addSnackBarForPrompt(
    for login: PasswordForm,
    tab: Tab,
    isUpdating: Bool,
    _ completion: @escaping () -> Void
  ) {
    guard let username = login.usernameValue else {
      return
    }

    // Remove the existing prompt
    if let existingPrompt = self.snackBar {
      tab.removeSnackbar(existingPrompt)
    }

    let promptMessage = String(
      format: isUpdating ? Strings.updateLoginUsernamePrompt : Strings.saveLoginUsernamePrompt,
      username,
      login.displayURLString
    )

    snackBar = TimerSnackBar(
      text: promptMessage,
      img: isUpdating
        ? UIImage(named: "key", in: .module, compatibleWith: nil)!
        : UIImage(named: "shields-menu-icon", in: .module, compatibleWith: nil)!
    )

    let dontSaveORUpdate = SnackButton(
      title: isUpdating
        ? Strings.loginsHelperDontUpdateButtonTitle : Strings.loginsHelperDontSaveButtonTitle,
      accessibilityIdentifier: "UpdateLoginPrompt.dontSaveUpdateButton"
    ) { [unowned self] bar in
      tab.removeSnackbar(bar)
      self.snackBar = nil
      return
    }

    let saveORUpdate = SnackButton(
      title: isUpdating
        ? Strings.loginsHelperUpdateButtonTitle : Strings.loginsHelperSaveLoginButtonTitle,
      accessibilityIdentifier: "UpdateLoginPrompt.saveUpdateButton"
    ) { [unowned self] bar in
      tab.removeSnackbar(bar)
      self.snackBar = nil

      completion()
    }

    snackBar?.addButton(dontSaveORUpdate)
    snackBar?.addButton(saveORUpdate)

    if let bar = snackBar {
      tab.addSnackbar(bar)
    }
  }

  private func autoFillRequestedCredentials(
    formSubmitURL: String,
    logins: [PasswordForm],
    requestId: String,
    frameInfo: WKFrameInfo,
    tab: Tab
  ) {
    let securityOrigin = frameInfo.securityOrigin

    var jsonObj = [String: Any]()
    jsonObj["requestId"] = requestId
    jsonObj["name"] = "RemoteLogins:loginsFound"
    jsonObj["logins"] = logins.compactMap { loginData -> [String: String]? in
      if frameInfo.isMainFrame {
        return loginData.toDict(formSubmitURL: formSubmitURL)
      }

      // Check for current tab has a url to begin with
      // and the frame is not modified
      guard let currentURL = tab.webView?.url,
        LoginsScriptHandler.checkIsSameFrame(
          url: currentURL,
          frameScheme: securityOrigin.protocol,
          frameHost: securityOrigin.host,
          framePort: securityOrigin.port
        )
      else {
        return nil
      }

      // If it is not the main frame, return username only, but no password!
      // Chromium does the same on iOS.
      // Firefox does NOT support third-party frames or iFrames.
      if let updatedLogin = loginData.copy() as? PasswordForm {
        updatedLogin.update(loginData.usernameValue, passwordValue: "")

        return updatedLogin.toDict(formSubmitURL: formSubmitURL)
      }

      return nil
    }

    let json = JSON(jsonObj)
    guard let jsonString = json.stringValue() else {
      return
    }

    tab.webView?.evaluateSafeJavaScript(
      functionName: "window.__firefox__.logins.inject",
      args: [jsonString],
      contentWorld: LoginsScriptHandler.scriptSandbox,
      escapeArgs: false
    ) { (object, error) -> Void in
      if let error = error {
        Logger.module.error("\(error.localizedDescription, privacy: .public)")
      }
    }
  }
}

extension LoginsScriptHandler {

  /// Helper method for checking if frame security origin elements are same as url from the webview
  /// - Parameters:
  ///   - url: url of the webview / tab
  ///   - frameScheme: Scheme of frameInfo
  ///   - frameHost: Host of frameInfo
  ///   - framePort: Port of frameInfo
  /// - Returns: Boolean indicating url and frameInfo has same elements
  static func checkIsSameFrame(
    url: URL,
    frameScheme: String,
    frameHost: String,
    framePort: Int
  ) -> Bool {
    // Prevent XSS on non main frame
    // Check the frame origin host belongs to the same security origin host
    guard let currentHost = url.host, !currentHost.isEmpty, currentHost == frameHost else {
      return false
    }

    // Check port for frame origin exists
    // and belongs to the same security origin port
    if let currentPort = url.port, currentPort != framePort {
      return false
    }

    if url.port == nil, framePort != 0 {
      return false
    }

    // Check scheme exists for frame origin
    // and belongs to the same security origin protocol
    if let currentScheme = url.scheme, currentScheme != frameScheme {
      return false
    }

    return true
  }
}
