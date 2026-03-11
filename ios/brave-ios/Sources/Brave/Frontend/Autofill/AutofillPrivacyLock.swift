// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import LocalAuthentication
import SwiftUI

private struct AutofillPrivacyLockEnvironmentKey: EnvironmentKey {
  static let defaultValue: AutofillPrivacyLock? = nil
}

extension EnvironmentValues {
  /// Shared privacy lock for the autofill management hierarchy.
  /// The root ManagePasswordsView owns the instance; child views inherit it via environment
  /// so they show the privacy overlay when backgrounded without running their own auth challenge on appear.
  var autofillPrivacyLock: AutofillPrivacyLock? {
    get { self[AutofillPrivacyLockEnvironmentKey.self] }
    set { self[AutofillPrivacyLockEnvironmentKey.self] = newValue }
  }
}

/// Owns the biometric/passcode authentication state for the autofill management hierarchy.
/// The root ManagePasswordsView creates a single instance and injects it via environment;
/// child views (ManagePasswordGroupView, ManagePasswordDetailView) inherit it and show the
/// privacy overlay when locked, without running their own auth challenge on appear.
///
/// Root usage:
/// ```swift
/// @State private var lock = AutofillPrivacyLock()
/// .environment(\.autofillPrivacyLock, lock)
/// .overlay { if lock.isLocked { Color(.braveGroupedBackground).ignoresSafeArea() } }
/// .task { await lock.authenticate(onFailure: dismiss) }
/// .onReceive(willDeactivate) { lock.lock() }
/// .onReceive(didActivate)    { Task { await lock.authenticate(onFailure: dismiss) } }
/// ```
@MainActor
@Observable
final class AutofillPrivacyLock {
  private(set) var isAuthenticated = false
  private(set) var isAuthenticating = false

  /// True whenever the privacy overlay should be shown.
  var isLocked: Bool { !isAuthenticated || isAuthenticating }

  /// Resets authentication state, causing the overlay to reappear.
  func lock() { isAuthenticated = false }

  /// Presents a biometric/passcode prompt if not already authenticated or authenticating.
  /// - Parameter onFailure: Called when the prompt is cancelled or fails; typically `dismiss`.
  func authenticate(onFailure: @escaping () -> Void) async {
    guard !isAuthenticated, !isAuthenticating else { return }
    isAuthenticating = true
    defer { isAuthenticating = false }
    do {
      let context = LAContext()
      guard context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) else {
        isAuthenticated = true
        return
      }
      isAuthenticated = try await context.evaluatePolicy(
        .deviceOwnerAuthentication,
        localizedReason: Strings.Autofill.managePasswordsAuthenticationReason
      )
    } catch {
      onFailure()
    }
  }
}
