// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import OSLog
import Preferences
import Strings
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct LoginsTabHelperKey: TabDataKey {
    static var defaultValue: LoginsTabHelper?
  }
  var logins: LoginsTabHelper? {
    get { self[LoginsTabHelperKey.self] }
    set { self[LoginsTabHelperKey.self] = newValue }
  }
}

class LoginsTabHelper: TabObserver, LoginsTabHelperBridge {
  private weak var tab: (any TabState)?
  private let passwordAPI: BravePasswordAPI
  private var snackBar: SnackBar?

  init(tab: some TabState, passwordAPI: BravePasswordAPI) {
    self.tab = tab
    self.passwordAPI = passwordAPI

    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: - TabObserver

  func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.setLoginsHelper(self)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - LoginsTabHelperBridge

  func handleFormSubmit(_ credentialsJSON: String) {
    guard let tab, let url = tab.lastCommittedURL else { return }
    if Preferences.General.saveLogins.value {
      do {
        if let script = try JSONSerialization.jsonObject(with: Data(credentialsJSON.utf8))
          as? [String: Any]
        {
          updateOrSaveCredentials(for: url, script: script, tab: tab)
        }
      } catch {
        Logger.module.error("Failed to parse Logins script JSON: \(error)")
      }
    }
  }

  // MARK: -

  func updateOrSaveCredentials(for url: URL, script: [String: Any], tab: any TabState) {
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

  private func showAddPrompt(for login: PasswordForm, tab: some TabState) {
    addSnackBarForPrompt(for: login, tab: tab, isUpdating: false) { [weak self] in
      guard let self = self else { return }

      DispatchQueue.main.async {
        self.passwordAPI.addLogin(login)
      }
    }
  }

  private func showUpdatePrompt(from old: PasswordForm, to new: PasswordForm, tab: some TabState) {
    addSnackBarForPrompt(for: new, tab: tab, isUpdating: true) { [weak self] in
      guard let self = self else { return }

      self.passwordAPI.updateLogin(new, oldPasswordForm: old)
    }
  }

  private func addSnackBarForPrompt(
    for login: PasswordForm,
    tab: some TabState,
    isUpdating: Bool,
    _ completion: @escaping () -> Void
  ) {
    guard let username = login.usernameValue else {
      return
    }
    let snackBarTabHelper = SnackBarTabHelper.from(tab: tab)

    // Remove the existing prompt
    if let existingPrompt = self.snackBar {
      snackBarTabHelper?.removeSnackbar(existingPrompt)
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
    ) { [weak self] bar in
      guard let self else { return }
      snackBarTabHelper?.removeSnackbar(bar)
      self.snackBar = nil
    }

    let saveORUpdate = SnackButton(
      title: isUpdating
        ? Strings.loginsHelperUpdateButtonTitle : Strings.loginsHelperSaveLoginButtonTitle,
      accessibilityIdentifier: "UpdateLoginPrompt.saveUpdateButton"
    ) { [weak self] bar in
      guard let self else { return }
      snackBarTabHelper?.removeSnackbar(bar)
      self.snackBar = nil

      completion()
    }

    snackBar?.addButton(dontSaveORUpdate)
    snackBar?.addButton(saveORUpdate)

    if let bar = snackBar {
      snackBarTabHelper?.addSnackbar(bar)
    }
  }
}
