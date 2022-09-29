/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import Storage
import UIKit

private let log = Logger.browserLogger

class Authenticator {
  fileprivate static let maxAuthenticationAttempts = 3

  static func handleAuthRequest(_ viewController: UIViewController, challenge: URLAuthenticationChallenge, loginsHelper: LoginsScriptHandler?) async throws -> LoginData {
    // If there have already been too many login attempts, we'll just fail.
    if challenge.previousFailureCount >= Authenticator.maxAuthenticationAttempts {
      throw LoginDataError(description: "Too many attempts to open site")
    }

    var credential = challenge.proposedCredential

    // If we were passed an initial set of credentials from iOS, try and use them.
    if let proposed = credential {
      if !(proposed.user?.isEmpty ?? true) {
        if challenge.previousFailureCount == 0 {
          return Login.createWithCredential(credential!, protectionSpace: challenge.protectionSpace)
        }
      } else {
        credential = nil
      }
    }

    // If we have some credentials, we'll show a prompt with them.
    if let credential = credential {
      return try await promptForUsernamePassword(viewController, credentials: credential, protectionSpace: challenge.protectionSpace, loginsHelper: loginsHelper)
    }

    // Otherwise, try to look them up and show the prompt.
    if let loginsHelper = loginsHelper {
      let credentials = await findMatchingCredentialsForChallenge(challenge, fromLoginsProvider: loginsHelper.logins)
      return try await self.promptForUsernamePassword(viewController, credentials: credentials, protectionSpace: challenge.protectionSpace, loginsHelper: loginsHelper)
    }

    // No credentials, so show an empty prompt.
    return try await self.promptForUsernamePassword(viewController, credentials: nil, protectionSpace: challenge.protectionSpace, loginsHelper: nil)
  }

  static func findMatchingCredentialsForChallenge(_ challenge: URLAuthenticationChallenge, fromLoginsProvider loginsProvider: BrowserLogins) async -> URLCredential? {
    guard let cursor = try? await loginsProvider.getLoginsForProtectionSpace(challenge.protectionSpace),
          cursor.count >= 1 else {
      return nil
    }
    
    let logins = cursor.asArray()
    var credentials: URLCredential?
    
    // It is possible that we might have duplicate entries since we match against host and scheme://host.
    // This is a side effect of https://bugzilla.mozilla.org/show_bug.cgi?id=1238103.
    if logins.count > 1 {
      credentials =
      (logins.find { login in
        (login.protectionSpace.`protocol` == challenge.protectionSpace.`protocol`) && !login.hasMalformedHostname
      })?.credentials
      
      let malformedGUIDs: [GUID] = logins.compactMap { login in
        if login.hasMalformedHostname {
          return login.guid
        }
        return nil
      }
      
      do {
        try await loginsProvider.removeLoginsWithGUIDs(malformedGUIDs)
        log.debug("Removed malformed logins.")
      } catch {
        log.error("Failed to remove malformed logins.")
      }
    }
    
    // Found a single entry but the schemes don't match. This is a result of a schemeless entry that we
    // saved in a previous iteration of the app so we need to migrate it. We only care about the
    // the username/password so we can rewrite the scheme to be correct.
    else if logins.count == 1 && logins[0].protectionSpace.`protocol` != challenge.protectionSpace.`protocol` {
      let login = logins[0]
      credentials = login.credentials
      let new = Login(credential: login.credentials, protectionSpace: challenge.protectionSpace)
      do {
        try await loginsProvider.updateLoginByGUID(login.guid, new: new, significant: true)
        return credentials
      } catch { }
    }
    
    // Found a single entry that matches the scheme and host - good to go.
    else {
      credentials = logins[0].credentials
    }
    
    return credentials
  }

  @MainActor fileprivate static func promptForUsernamePassword(_ viewController: UIViewController, credentials: URLCredential?, protectionSpace: URLProtectionSpace, loginsHelper: LoginsScriptHandler?) async throws -> LoginData {
    if protectionSpace.host.isEmpty {
      print("Unable to show a password prompt without a hostname")
      throw LoginDataError(description: "Unable to show a password prompt without a hostname")
    }

    return try await withCheckedThrowingContinuation { continuation in
      let alert: AlertController
      
      if !(protectionSpace.realm?.isEmpty ?? true) {
        let formatted = String(format: Strings.authPromptAlertFormatRealmMessageText, protectionSpace.host, protectionSpace.realm ?? "")
        alert = AlertController(title: Strings.authPromptAlertTitle, message: formatted, preferredStyle: .alert)
      } else {
        
        let formatted = String(format: Strings.authPromptAlertMessageText, protectionSpace.host)
        alert = AlertController(title: Strings.authPromptAlertTitle, message: formatted, preferredStyle: .alert)
      }
      
      // Add a button to log in.
      let action = UIAlertAction(
        title: Strings.authPromptAlertLogInButtonTitle,
        style: .default
      ) { (action) -> Void in
        guard let user = alert.textFields?[0].text, let pass = alert.textFields?[1].text else {
          continuation.resume(throwing: LoginDataError(description: "Username and Password required"))
          return
        }
        
        let login = Login.createWithCredential(URLCredential(user: user, password: pass, persistence: .forSession), protectionSpace: protectionSpace)
        continuation.resume(returning: login)
        loginsHelper?.setCredentials(login)
      }
      alert.addAction(action, accessibilityIdentifier: "authenticationAlert.loginRequired")
      
      // Add a cancel button.
      let cancel = UIAlertAction(title: Strings.authPromptAlertCancelButtonTitle, style: .cancel) { (action) -> Void in
        continuation.resume(throwing: LoginDataError(description: "Save password cancelled"))
      }
      alert.addAction(cancel, accessibilityIdentifier: "authenticationAlert.cancel")
      
      // Add a username textfield.
      alert.addTextField { (textfield) -> Void in
        textfield.placeholder = Strings.authPromptAlertUsernamePlaceholderText
        textfield.text = credentials?.user
      }
      
      // Add a password textfield.
      alert.addTextField { (textfield) -> Void in
        textfield.placeholder = Strings.authPromptAlertPasswordPlaceholderText
        textfield.isSecureTextEntry = true
        textfield.text = credentials?.password
      }
      
      viewController.present(alert, animated: true) { () -> Void in }
    }
  }

}
