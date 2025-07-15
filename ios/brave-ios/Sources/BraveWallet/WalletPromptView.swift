// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Shared
import SwiftUI

struct WalletPromptContentView<Content, Footer>: View where Content: View, Footer: View {
  let content: () -> Content
  var primaryButton: WalletPromptButton
  @Binding var isPrimaryButtonEnabled: Bool
  var secondaryButton: WalletPromptButton?
  var buttonsAxis: Axis
  let showCloseButton: Bool
  let dismissAction: (() -> Void)?
  let footer: () -> Footer

  init(
    primaryButton: WalletPromptButton,
    isPrimaryButtonEnabled: Binding<Bool>,
    secondaryButton: WalletPromptButton?,
    buttonsAxis: Axis,
    showCloseButton: Bool,
    dismissAction: (() -> Void)?,
    @ViewBuilder content: @escaping () -> Content,
    @ViewBuilder footer: @escaping () -> Footer
  ) {
    self.primaryButton = primaryButton
    self._isPrimaryButtonEnabled = isPrimaryButtonEnabled
    self.secondaryButton = secondaryButton
    self.buttonsAxis = buttonsAxis
    self.showCloseButton = showCloseButton
    self.dismissAction = dismissAction
    self.content = content
    self.footer = footer
  }

  var body: some View {
    VStack {
      content()
      if let secondaryButton = self.secondaryButton {
        if buttonsAxis == .vertical {
          VStack(spacing: 24) {
            Button {
              primaryButton.action(nil)
            } label: {
              Text(primaryButton.title)
                .font(.footnote.weight(.semibold))
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
            .disabled(!isPrimaryButtonEnabled)
            Button {
              secondaryButton.action(nil)
            } label: {
              Text(secondaryButton.title)
                .font(.footnote.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
                .frame(maxWidth: .infinity)
            }
          }
        } else {
          HStack {
            Button {
              secondaryButton.action(nil)
            } label: {
              Text(secondaryButton.title)
                .font(.footnote.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(BraveOutlineButtonStyle(size: .large))
            Button {
              primaryButton.action(nil)
            } label: {
              Text(primaryButton.title)
                .font(.footnote.weight(.semibold))
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
            .disabled(!isPrimaryButtonEnabled)
          }
        }
      } else {
        Button {
          primaryButton.action(nil)
        } label: {
          Text(primaryButton.title)
            .font(.footnote.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .disabled(!isPrimaryButtonEnabled)
      }
      footer()
    }
    .frame(maxWidth: .infinity)
    .padding(.horizontal, 24)
    .padding(.vertical, 32)
    .overlay(
      showCloseButton
        ? Button {
          dismissAction?()
        } label: {
          Image(systemName: "xmark")
            .padding(16)
        }
        .font(.headline)
        .foregroundColor(.gray)
        : nil,
      alignment: .topTrailing
    )
    .accessibilityEmbedInScrollView()
  }
}

struct WalletPromptButton {
  let title: String
  let action: (UINavigationController?) -> Void
}

struct WalletPromptView<Content, Footer>: UIViewControllerRepresentable
where Content: View, Footer: View {
  @Binding var isPresented: Bool
  @Binding var isPrimaryButtonEnabled: Bool
  var primaryButton: WalletPromptButton
  var secondaryButton: WalletPromptButton?
  var buttonsAxis: Axis
  var showCloseButton: Bool
  var dismissAction: ((UINavigationController?) -> Void)?
  var content: () -> Content
  var footer: () -> Footer

  init(
    isPresented: Binding<Bool>,
    isPrimaryButtonEnabled: Binding<Bool>,
    primaryButton: WalletPromptButton,
    secondaryButton: WalletPromptButton? = nil,
    buttonsAxis: Axis = .vertical,
    showCloseButton: Bool = true,
    dismissAction: ((UINavigationController?) -> Void)? = nil,
    @ViewBuilder content: @escaping () -> Content,
    @ViewBuilder footer: @escaping () -> Footer
  ) {
    _isPresented = isPresented
    _isPrimaryButtonEnabled = isPrimaryButtonEnabled
    self.primaryButton = primaryButton
    self.secondaryButton = secondaryButton
    self.buttonsAxis = buttonsAxis
    self.showCloseButton = showCloseButton
    self.dismissAction = dismissAction
    self.content = content
    self.footer = footer
  }

  func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }

  func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }
      let newPrimaryButton = WalletPromptButton(
        title: primaryButton.title,
        action: { _ in
          primaryButton.action(uiViewController.navigationController)
        }
      )
      var newSecodaryButton: WalletPromptButton?
      if let button = secondaryButton {
        newSecodaryButton = WalletPromptButton(
          title: button.title,
          action: { _ in
            button.action(uiViewController.navigationController)
          }
        )
      }
      let controller = PopupViewController(
        rootView: WalletPromptContentView(
          primaryButton: newPrimaryButton,
          isPrimaryButtonEnabled: $isPrimaryButtonEnabled,
          secondaryButton: newSecodaryButton,
          buttonsAxis: buttonsAxis,
          showCloseButton: showCloseButton,
          dismissAction: {
            dismissAction?(uiViewController.navigationController)
          },
          content: content,
          footer: footer
        )
      )
      context.coordinator.presentedWalletPrompt = true
      uiViewController.present(controller, animated: true)
    } else {
      if context.coordinator.presentedWalletPrompt {
        context.coordinator.presentedWalletPrompt = false
        uiViewController.presentedViewController?.dismiss(animated: true)
      }
    }
  }

  class Coordinator {
    var presentedWalletPrompt: Bool = false
  }

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }
}

extension WalletPromptView where Content: View, Footer == EmptyView {
  init(
    isPresented: Binding<Bool>,
    isPrimaryButtonEnabled: Binding<Bool> = .constant(true),
    primaryButton: WalletPromptButton,
    secondaryButton: WalletPromptButton? = nil,
    buttonsAxis: Axis = .vertical,
    showCloseButton: Bool = true,
    dismissAction: ((UINavigationController?) -> Void)? = nil,
    @ViewBuilder content: @escaping () -> Content
  ) {
    _isPresented = isPresented
    _isPrimaryButtonEnabled = isPrimaryButtonEnabled
    self.primaryButton = primaryButton
    self.secondaryButton = secondaryButton
    self.buttonsAxis = buttonsAxis
    self.showCloseButton = showCloseButton
    self.dismissAction = dismissAction
    self.content = content
    self.footer = { EmptyView() }
  }
}
