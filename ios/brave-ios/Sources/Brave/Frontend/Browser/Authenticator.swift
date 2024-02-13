/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit
import os.log
import BraveShared

class Authenticator {
  
  class LoginData {
    var guid: String
    var credentials: URLCredential
    let protectionSpace: URLProtectionSpace
      
    init(credentials: URLCredential, protectionSpace: URLProtectionSpace) {
      self.guid = Bytes.generateGUID()
      self.credentials = credentials
      self.protectionSpace = protectionSpace
    }
    
  }

  class LoginDataError: Error {
    let description: String
    init(description: String) {
      self.description = description
    }
  }
  
  private static let maxAuthenticationAttempts = 3

  static func handleAuthRequest(
    _ viewController: UIViewController,
    credential: URLCredential?,
    protectionSpace: URLProtectionSpace,
    previousFailureCount: Int
  ) async throws -> LoginData {
    var credential = credential
    // If there have already been too many login attempts, we'll just fail.
    if previousFailureCount >= Authenticator.maxAuthenticationAttempts {
      throw LoginDataError(description: "Too many attempts to open site")
    }

    // If we were passed an initial set of credentials from iOS, try and use them.
    if let proposed = credential {
      if !(proposed.user?.isEmpty ?? true) {
        if previousFailureCount == 0 {
          return LoginData(credentials: proposed, protectionSpace: protectionSpace)
        }
      } else {
        credential = nil
      }
    }

    // If we have some credentials, we'll show a prompt with them.
    if let credential = credential {
      return try await promptForUsernamePassword(viewController, credentials: credential, protectionSpace: protectionSpace)
    }

    // No credentials, so show an empty prompt.
    return try await self.promptForUsernamePassword(viewController, credentials: nil, protectionSpace: protectionSpace)
  }

  @MainActor private static func promptForUsernamePassword(_ viewController: UIViewController, credentials: URLCredential?, protectionSpace: URLProtectionSpace) async throws -> LoginData {
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
      let action = UIAlertAction(title: Strings.authPromptAlertLogInButtonTitle, style: .default) { _ in
        guard let user = alert.textFields?[0].text, let pass = alert.textFields?[1].text else {
          continuation.resume(throwing: LoginDataError(description: "Username and Password required"))
          return
        }
        
        let loginData = LoginData(credentials: URLCredential(user: user, password: pass, persistence: .forSession), protectionSpace: protectionSpace)
        continuation.resume(returning: loginData)
      }
      alert.addAction(action, accessibilityIdentifier: "authenticationAlert.loginRequired")
      
      // Add a cancel button.
      let cancel = UIAlertAction(title: Strings.authPromptAlertCancelButtonTitle, style: .cancel) { _ in
        continuation.resume(throwing: LoginDataError(description: "Save password cancelled"))
      }
      alert.addAction(cancel, accessibilityIdentifier: "authenticationAlert.cancel")
      
      // Add a username textfield.
      alert.addTextField { textfield in
        textfield.placeholder = Strings.authPromptAlertUsernamePlaceholderText
        textfield.text = credentials?.user
      }
      
      // Add a password textfield.
      alert.addTextField { textfield in
        textfield.placeholder = Strings.authPromptAlertPasswordPlaceholderText
        textfield.isSecureTextEntry = true
        textfield.text = credentials?.password
      }
      
      viewController.present(alert, animated: true)
    }
  }
}
