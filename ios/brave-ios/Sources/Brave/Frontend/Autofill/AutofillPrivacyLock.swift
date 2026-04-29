// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import LocalAuthentication
import SwiftUI

/// Biometric/passcode gate for autofill management screens.
/// `ManagePasswordsView` keeps one `@State` instance and combines `lock()` / `authenticate(onFailure:)`
/// with scene lifecycle (`didEnterBackground` / `willEnterForeground`). Child screens get privacy styling
/// via `\.redactionReasons` (and related modifiers), not via this type in the environment.
///
/// ```swift
/// @State private var lock = AutofillPrivacyLock()
/// .onAppear { Task { await lock.authenticate(onFailure: exit) } }
/// .onReceive(didEnterBackground) { lock.lock() }
/// .onReceive(willEnterForeground) { Task { await lock.authenticate(onFailure: exit) } }
/// ```
@MainActor
@Observable
final class AutofillPrivacyLock {
  private(set) var isAuthenticated = false
  private(set) var isAuthenticating = false

  /// True when re-authentication is required (not authenticated or prompt in flight).
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

struct AutofillPrivacyLockExitOnFailureAction {
  private var handler: () -> Void

  public init(handler: @escaping () -> Void) {
    self.handler = handler
  }

  public func callAsFunction() {
    handler()
  }
}

extension EnvironmentValues {
  @Entry var autofillPrivacyLockExitOnFailure: AutofillPrivacyLockExitOnFailureAction?
}
