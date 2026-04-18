// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import OSLog
@_spi(ChromiumWebViewAccess) import Web

/// Handles `CWVAutofillControllerDelegate` callbacks for Chromium web views (see brave-browser#52216).
///
/// All optional protocol methods are stubbed out to rule out missing delegate
/// methods as a source of crashes while wiring up the Chromium autofill stack.
final class AutofillManager: NSObject, CWVAutofillControllerDelegate {

  /// Assigns this manager as the autofill delegate for `tab` when Chromium autofill is enabled.
  func attachIfNeeded(to tab: some TabState) {
    guard FeatureList.kUseChromiumWebViewsAutofill.enabled,
      let webView = BraveWebView.from(tab: tab)
    else {
      return
    }
    webView.autofillController.delegate = self
  }

  // MARK: - Field events

  func autofillController(
    _ autofillController: CWVAutofillController,
    didFocusOnFieldWithIdentifier fieldIdentifier: String,
    fieldType: String,
    formName: String,
    frameID: String,
    value: String,
    userInitiated: Bool
  ) {
    Logger.module.debug(
      "Autofill field focus: id=\(fieldIdentifier, privacy: .public) type=\(fieldType, privacy: .public) form=\(formName, privacy: .public) userInitiated=\(userInitiated)"
    )
  }

  func autofillController(
    _ autofillController: CWVAutofillController,
    didInputInFieldWithIdentifier fieldIdentifier: String,
    fieldType: String,
    formName: String,
    frameID: String,
    value: String,
    userInitiated: Bool
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    didBlurOnFieldWithIdentifier fieldIdentifier: String,
    fieldType: String,
    formName: String,
    frameID: String,
    value: String,
    userInitiated: Bool
  ) {}

  // MARK: - Form lifecycle

  func autofillController(
    _ autofillController: CWVAutofillController,
    didSubmitFormWithName formName: String,
    frameID: String,
    userInitiated: Bool,
    perfectFilling: Bool
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    didSubmitFormWithName formName: String,
    frameID: String,
    perfectFilling: Bool
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    didFind forms: [CWVAutofillForm],
    frameID: String
  ) {}

  // MARK: - Credit cards

  func autofillController(
    _ autofillController: CWVAutofillController,
    saveCreditCardWith saver: CWVCreditCardSaver
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    verifyCreditCardWith verifier: CWVCreditCardVerifier
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    verifyCreditCardWith otpVerifier: CWVCreditCardOTPVerifier
  ) {}

  // MARK: - Passwords

  func autofillController(
    _ autofillController: CWVAutofillController,
    decideSavePolicyFor password: CWVPassword,
    decisionHandler: @escaping (CWVPasswordUserDecision) -> Void
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    decideUpdatePolicyFor password: CWVPassword,
    decisionHandler: @escaping (CWVPasswordUserDecision) -> Void
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    notifyUserOfPasswordLeakOn url: URL,
    leakType: CWVPasswordLeakType
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    notifyUserOfPasswordLeakOn url: URL,
    leakType: CWVPasswordLeakType,
    username: String
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    suggestGeneratedPassword generatedPassword: String,
    decisionHandler: @escaping (Bool) -> Void
  ) {}

  func autofillControllerDidLogin(
    withExistingPassword autofillController: CWVAutofillController
  ) {}

  // MARK: - Profiles

  func autofillController(
    _ autofillController: CWVAutofillController,
    confirmSaveForNewAutofillProfile newProfile: CWVAutofillProfile,
    oldProfile: CWVAutofillProfile?,
    decisionHandler: @escaping (CWVAutofillProfileUserDecision) -> Void
  ) {}

  // MARK: - Progress / risk / VCN

  func autofillController(
    _ autofillController: CWVAutofillController,
    showProgressDialogOf type: CWVAutofillProgressDialogType,
    cancelAction: @escaping () -> Void
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    closeProgressDialogWithConfirmation showConfirmation: Bool,
    completion: @escaping () -> Void
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    showUnmaskCreditCardAuthenticator options: [CWVCardUnmaskChallengeOption],
    acceptBlock: @escaping (String) -> Void,
    cancelBlock: @escaping () -> Void
  ) {}

  func autofillControllerLoadRiskData(
    _ autofillController: CWVAutofillController,
    riskDataHandler handler: @escaping (String) -> Void
  ) {}

  func autofillController(
    _ autofillController: CWVAutofillController,
    enrollCreditCardWith enrollmentManager: CWVVCNEnrollmentManager
  ) {}
}
