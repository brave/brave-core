// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Combine
import DesignSystem
import Foundation
import LocalAuthentication
import Preferences
import Shared
import SwiftUI
import UIKit
import os.log

public enum AuthViewType {
  case external, general, sync, tabTray, passwords
}

public class WindowProtection {

  @Observable
  fileprivate class ViewModel {
    var isCancellable: Bool
    var isUnlockButtonHidden: Bool

    var onUnlock: (() -> Void)?
    var onCancel: (() -> Void)?

    init(isCancellable: Bool = false, isUnlockButtonHidden: Bool = false) {
      self.isCancellable = isCancellable
      self.isUnlockButtonHidden = isUnlockButtonHidden
    }
  }

  fileprivate struct LockedView: View {
    var viewModel: ViewModel

    var body: some View {
      VStack(spacing: 48) {
        Image("browser-lock-icon", bundle: .module)
        VStack {
          Button {
            viewModel.onUnlock?()
          } label: {
            Text(Strings.unlockButtonTitle)
              .frame(maxWidth: .infinity)
          }
          .buttonStyle(.filled)
          .opacity(viewModel.isUnlockButtonHidden ? 0 : 1)
          if viewModel.isCancellable {
            Button {
              viewModel.onCancel?()
            } label: {
              Text(Strings.cancelButtonTitle)
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(.plainFaint)
          }
        }
        .frame(maxWidth: 500)
      }
      .padding(24)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(.thickMaterial)
    }
  }

  private class LockedViewController: UIHostingController<LockedView> {
    init(viewModel: ViewModel) {
      super.init(rootView: LockedView(viewModel: viewModel))
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    override func viewDidLoad() {
      super.viewDidLoad()
      view.backgroundColor = .clear
    }
  }

  private let lockedViewController: LockedViewController
  private let viewModel: ViewModel = .init()

  private var cancellables: Set<AnyCancellable> = []
  private var passcodeWindow: UIWindow
  private var context = LAContext()
  private var viewType: AuthViewType = .general

  private var isVisible: Bool = false {
    didSet {
      passcodeWindow.isHidden = !isVisible
      // Prior window will be made key automatically after passcode window is dismissed
      if isVisible {
        passcodeWindow.makeKeyAndVisible()
      }
    }
  }

  var isPassCodeAvailable: Bool {
    var error: NSError?
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
      (error as? LAError)?.code == .passcodeNotSet
    {
      return false
    }

    return true
  }

  var isCancellable: Bool = false {
    didSet {
      viewModel.isCancellable = isCancellable
    }
  }

  private let onCancelPressed = PassthroughSubject<Void, Never>()
  private let didFinalizeAuthentication = PassthroughSubject<(Bool, AuthViewType), Never>()

  var cancelPressed: AnyPublisher<Void, Never> {
    onCancelPressed.eraseToAnyPublisher()
  }

  var finalizedAuthentication: AnyPublisher<(Bool, AuthViewType), Never> {
    didFinalizeAuthentication.eraseToAnyPublisher()
  }

  public init(windowScene: UIWindowScene) {
    lockedViewController = LockedViewController(viewModel: viewModel)

    passcodeWindow = UIWindow(windowScene: windowScene)
    passcodeWindow.windowLevel = .init(UIWindow.Level.statusBar.rawValue + 1)
    passcodeWindow.rootViewController = lockedViewController

    viewModel.onUnlock = { [unowned self] in
      tappedUnlock()
    }
    viewModel.onCancel = { [unowned self] in
      tappedCancel()
    }

    NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        // Should set as non cancallable for browser lock
        self.isCancellable = false
        // Update visibility when entering background
        self.isVisible =
          Preferences.Privacy.lockWithPasscode.value
          && self.context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil)
      })
      .store(in: &cancellables)

    NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        self.context = LAContext()  // Reset context for new session
        self.updateVisibleStatusForForeground(viewType: .external)
      })
      .store(in: &cancellables)

    updateVisibleStatusForForeground(viewType: .external)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc private func tappedUnlock() {
    presentLocalAuthentication(viewType: viewType)
  }

  @objc private func tappedCancel() {
    onCancelPressed.send(())
    isVisible = false
  }

  private func updateVisibleStatusForForeground(
    _ determineLockWithPasscode: Bool = true,
    viewType: AuthViewType = .general,
    completion: ((Bool, LAError.Code?) -> Void)? = nil
  ) {
    var error: NSError?
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
      (error as? LAError)?.code == .passcodeNotSet
    {
      // User no longer has a passcode set so we can't evaluate the auth policy
      isVisible = false
      completion?(false, .passcodeNotSet)
      return
    }

    if determineLockWithPasscode {
      let isLocked = Preferences.Privacy.lockWithPasscode.value
      isVisible = isLocked
      if isLocked {
        presentLocalAuthentication(viewType: viewType) { status, error in
          completion?(status, error)
        }
      }
    } else {
      isVisible = true
      presentLocalAuthentication(viewType: viewType) { status, error in
        completion?(status, error)
      }
    }
  }

  private func presentLocalAuthentication(
    viewType: AuthViewType,
    completion: ((Bool, LAError.Code?) -> Void)? = nil
  ) {
    self.viewType = viewType

    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
      completion?(false, .passcodeNotSet)
      return
    }

    viewModel.isUnlockButtonHidden = true
    isCancellable = viewType != .external

    context.evaluatePolicy(
      .deviceOwnerAuthentication,
      localizedReason: Strings.authenticationLoginsTouchReason
    ) { success, error in
      DispatchQueue.main.async { [self] in
        if success {
          UIView.animate(
            withDuration: 0.1,
            animations: { [self] in
              lockedViewController.view.alpha = 0.0
            },
            completion: { [self] _ in
              isVisible = false
              lockedViewController.view.alpha = 1.0
              completion?(true, nil)
            }
          )
        } else {
          viewModel.isUnlockButtonHidden = false

          let errorPolicy = error as? LAError
          completion?(false, errorPolicy?.code)

          if let error = error {
            Logger.module.error(
              "Failed to unlock browser using local authentication: \(error.localizedDescription)"
            )
          }
        }

        self.didFinalizeAuthentication.send((success, viewType))
      }
    }
  }

  public func presentAuthenticationForViewController(
    determineLockWithPasscode: Bool = true,
    viewType: AuthViewType,
    completion: ((Bool, LAError.Code?) -> Void)? = nil
  ) {
    if isVisible {
      return
    }

    self.viewType = viewType

    context = LAContext()
    updateVisibleStatusForForeground(determineLockWithPasscode, viewType: viewType) {
      status,
      error in
      completion?(status, error)
    }
  }
}

#if DEBUG

#Preview {
  WindowProtection.LockedView(viewModel: .init())
}

#Preview("Cancellable") {
  WindowProtection.LockedView(viewModel: .init(isCancellable: true))
}

#Preview("Unlock Hidden") {
  WindowProtection.LockedView(viewModel: .init(isUnlockButtonHidden: true))
}

#endif
