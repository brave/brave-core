// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AuthenticationServices
import BraveCore
import CredentialProviderUI
import LocalAuthentication
import Security
import SwiftUI

class CredentialProviderViewController: ASCredentialProviderViewController {
  private let model = CredentialListModel()
  private var identifiers: [ASCredentialServiceIdentifier]?
  private lazy var credentialStore = CredentialProviderAPI.credentialStore()

  override func viewDidLoad() {
    super.viewDidLoad()

    model.actionHandler = { [unowned self] action in
      switch action {
      case .selectedCredential(let credential):
        self.completeRequestWithCredential(credential)
      case .cancelled:
        self.extensionContext.cancelRequest(
          withError: NSError(
            domain: ASExtensionErrorDomain,
            code: ASExtensionError.userCanceled.rawValue
          )
        )
      }
    }
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    // Delay setting up the UI until the view is actually being shown to the user, since this
    // VC is created even if a user is going through `prepareInterfaceForExtensionConfiguration` or
    // `provideCredentialWithoutUserInteraction` flows.
    if let identifiers {
      let hostingController = UIHostingController(rootView: CredentialListView(model: model))
      hostingController.modalPresentationStyle = .overCurrentContext
      present(hostingController, animated: false)
      model.populateFromStore(credentialStore, identifiers: identifiers)
    }
  }

  override func prepareCredentialList(for serviceIdentifiers: [ASCredentialServiceIdentifier]) {
    // Sometimes, this method is called while the authentication framework thinks the app is not
    // foregrounded, so authentication fails. Instead of directly authenticating and showing the
    // credentials, store the list of identifiers and authenticate once the extension is visible.
    identifiers = serviceIdentifiers
  }

  override func provideCredentialWithoutUserInteraction(
    for credentialIdentity: ASPasswordCredentialIdentity
  ) {
    guard
      isPasscodeAvailable,
      let credential = credentialStore.credential(
        withRecordIdentifier: credentialIdentity.recordIdentifier
      )
    else {
      extensionContext.cancelRequest(
        withError: NSError(
          domain: ASExtensionErrorDomain,
          code: ASExtensionError.userInteractionRequired.rawValue
        )
      )
      return
    }
    // iOS already gates the password with device auth for
    // provideCredentialWithoutUserInteraction(for:) so no need to show the UI to do it again.
    completeRequestWithCredential(credential)
  }

  override func prepareInterfaceToProvideCredential(
    for credentialIdentity: ASPasswordCredentialIdentity
  ) {
    // Called when `provideCredentialWithoutUserInteraction` fails with `userInteractionRequired`
    // Since our UI always shows authentication by default, we don't need to do anything special.
  }

  override func prepareInterfaceForExtensionConfiguration() {
    super.prepareInterfaceForExtensionConfiguration()

    let hostingController = UIHostingController(
      rootView: CredentialProviderOnboardingView(action: { [unowned self] in
        self.extensionContext.completeExtensionConfigurationRequest()
      })
    )
    hostingController.modalPresentationStyle = .fullScreen
    hostingController.isModalInPresentation = true
    present(hostingController, animated: false)
  }

  // MARK: -

  private func completeRequestWithCredential(_ item: any Credential) {
    guard let password = item.password else {
      extensionContext.cancelRequest(
        withError: NSError(
          domain: ASExtensionErrorDomain,
          code: ASExtensionError.failed.rawValue
        )
      )
      return
    }
    let credential = ASPasswordCredential(user: item.username, password: password)
    extensionContext.completeRequest(withSelectedCredential: credential)
  }

  private var isPasscodeAvailable: Bool {
    var error: NSError?
    let context = LAContext()
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
      (error as? LAError)?.code == .passcodeNotSet
    {
      return false
    }
    return true
  }
}
