// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Strings
import SwiftUI
import UIKit
import Web

struct QuickViewToolbarView: View {
  let viewModel: QuickViewToolbarModel
  /// An invisible `UIView` background lives in SwiftUI for UIKit API to reference later
  var shieldBackgroundView: InvisibleUIView = .init()
  var shareBackgroundView: InvisibleUIView = .init()
  var sslStatusBackgroundView: InvisibleUIView = .init()
  var browserColors: any BrowserColors {
    viewModel.isPrivate ? .privateMode : .standard
  }

  var body: some View {
    VStack(spacing: 0) {
      topRow
      bottomRow
        .padding(.top, 12)
    }
    .padding(16)
  }

  private var shieldButton: some View {
    Button {
      viewModel.onActionButton?(.shield)
    } label: {
      Label {
        Text(Strings.quickViewShieldAccessibilityLabel)
      } icon: {
        Image(sharedName: viewModel.isShieldEnabled ? "brave.logo" : "brave.logo.greyscale")
      }
    }
    .disabled(viewModel.readerModeState == .active)
  }

  @ViewBuilder
  private var secondaryTopButtonView: some View {
    if let button = viewModel.secondaryTopButton {
      switch button {
      case .playlist:
        Button {
          viewModel.onActionButton?(.playlist)
        } label: {
          Label(
            Strings.quickViewPlaylistAccessibilityLabel,
            braveSystemImage: "leo.product.playlist-add"
          )
          .tint(
            Color(braveSystemName: viewModel.isPlaylistEnabled ? .iconInteractive : .iconDefault)
          )
        }
      case .readerMode:
        Button {
          viewModel.onActionButton?(.readerMode)
        } label: {
          Label(
            Strings.quickViewReaderModeAccessibilityLabel,
            braveSystemImage: "leo.product.speedreader"
          )
          .tint(
            viewModel.readerModeState == .active
              ? Color(braveSystemName: .iconInteractive) : Color(braveSystemName: .iconDefault)
          )
        }
      case .translate:
        Button {
          viewModel.onActionButton?(.translate)
        } label: {
          Label(
            Strings.quickViewTranslateAccessibilityLabel,
            braveSystemImage: "leo.product.translate"
          )
          .tint(
            viewModel.isTranslateEnabled
              ? Color(braveSystemName: .iconInteractive) : Color(braveSystemName: .iconDefault)
          )
        }
      default:
        EmptyView()
      }
    }
  }

  private var refreshButton: some View {
    Button {
      viewModel.onActionButton?(.refresh)
    } label: {
      Label(Strings.quickViewRefreshAccessibilityLabel, braveSystemImage: "leo.browser.refresh")
        .tint(Color(braveSystemName: .iconDefault))
    }
  }

  private var addressView: some View {
    URLDisplayLabel(
      formattedURL: viewModel.displayURL,
      isLeftToRight: viewModel.isURLLeftToRight,
      textFont: UIFont.preferredFont(forTextStyle: .body),
      textColor: UIColor(braveSystemName: .textTertiary),
      gradientColors: [
        browserColors.chromeBackground.cgColor,
        browserColors.chromeBackground.withAlphaComponent(0.1).cgColor,
      ]
    )
    .frame(maxWidth: .infinity)
    .accessibilityLabel(viewModel.displayURL)
  }

  private var topRightButtonsView: some View {
    HStack(spacing: 12) {
      secondaryTopButtonView
      refreshButton
    }
  }

  @ViewBuilder private var warningIcon: some View {
    switch viewModel.secureContentState {
    case .secure, .localhost:
      EmptyView()
    case .unknown:
      Image(braveSystemName: "leo.info.filled")
        .foregroundColor(Color(braveSystemName: .iconDefault))
        .font(.caption2)
        .accessibilityLabel(Text(Strings.PageSecurityView.pageUnknownStatusTitle))
    case .invalidCertificate:
      Image(braveSystemName: "leo.warning.triangle-filled")
        .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
        .font(.footnote)
    case .missingSSL, .mixedContent:
      Image(braveSystemName: "leo.warning.triangle-filled")
        .foregroundColor(Color(braveSystemName: .textTertiary))
        .font(.footnote)
    }
  }

  @ViewBuilder private var warningTitle: some View {
    switch viewModel.secureContentState {
    case .secure, .localhost, .unknown:
      EmptyView()
    case .invalidCertificate:
      Text(Strings.tabToolbarNotSecureTitle)
        .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
        .font(.footnote)
    case .missingSSL, .mixedContent:
      Text(Strings.tabToolbarNotSecureTitle)
        .foregroundColor(Color(braveSystemName: .textTertiary))
        .font(.footnote)
    }
  }

  private var sslStatusButton: some View {
    Button {
      viewModel.onActionButton?(.sslStatus)
    } label: {
      HStack(spacing: 4) {
        warningIcon
        warningTitle
      }
    }
  }

  private var topRow: some View {
    HStack(alignment: .top, spacing: 8) {
      shieldButton
        .background(shieldBackgroundView)

      VStack(spacing: 12) {
        HStack(spacing: 8) {
          if viewModel.secureContentState.shouldDisplayWarning {
            sslStatusButton
              .background(sslStatusBackgroundView)
          }
          addressView
        }
        progressBar
          .hidden(isHidden: !viewModel.isLoading)
      }
      .padding(.horizontal, 16)

      topRightButtonsView
    }
    .labelStyle(QuickViewToolbarLabelTopIconStyle())
  }

  private var progressBar: some View {
    GeometryReader { geo in
      Color(braveSystemName: .iconInteractive)
        .frame(width: geo.size.width * viewModel.loadingProgress)
    }
    .frame(height: 2)
    .background(Color(braveSystemName: .containerHighlight))
    .cornerRadius(1)
    .animation(.easeInOut(duration: 0.2), value: viewModel.loadingProgress)
  }

  private var backButton: some View {
    Button {
      viewModel.onActionButton?(.back)
    } label: {
      Label(Strings.quickViewBackAccessibilityLabel, braveSystemImage: "leo.browser.back")
    }
    .disabled(!viewModel.canGoBack)
  }

  private var forwardButton: some View {
    Button {
      viewModel.onActionButton?(.forward)
    } label: {
      Label(Strings.quickViewForwardAccessibilityLabel, braveSystemImage: "leo.browser.forward")
    }
  }

  private var shareButton: some View {
    Button {
      viewModel.onActionButton?(.share)
    } label: {
      Label(Strings.quickViewShareAccessibilityLabel, braveSystemImage: "leo.share.macos")
    }
  }

  private var openInTabButton: some View {
    Button {
      viewModel.onActionButton?(.openTab)
    } label: {
      Label(Strings.quickViewOpenTabAccessibilityLabel, braveSystemImage: "leo.open.in-tab")
    }
  }

  private var closeButton: some View {
    Button {
      viewModel.onActionButton?(.close)
    } label: {
      Label(Strings.close, braveSystemImage: "leo.close")
    }
  }

  private var bottomRow: some View {
    HStack(spacing: 0) {
      backButton

      Spacer()

      if viewModel.canGoForward {
        forwardButton
      } else {
        shareButton
          .background(shareBackgroundView)
      }

      Spacer()

      openInTabButton

      Spacer()

      closeButton
    }
    .buttonStyle(QuickViewToolbarBottomButtonStyle())
    .labelStyle(
      QuickViewToolbarLabelBottomIconStyle(
        iconDefaultColor: Color(browserColors.iconDefault),
        iconDisabledColor: Color(browserColors.iconDisabled)
      )
    )
  }
}

private struct QuickViewToolbarLabelTopIconStyle: LabelStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .font(.headline)
      .accessibilityRepresentation {
        configuration.title
      }
  }
}

private struct QuickViewToolbarLabelBottomIconStyle: LabelStyle {
  let iconDefaultColor: Color
  let iconDisabledColor: Color
  @Environment(\.isEnabled) private var isEnabled

  func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .font(.title2)
      .foregroundStyle(isEnabled ? iconDefaultColor : iconDisabledColor)
      .accessibilityRepresentation {
        configuration.title
      }
  }
}

private struct QuickViewToolbarBottomButtonStyle: ButtonStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .opacity(configuration.isPressed ? 0.5 : 1.0)
  }
}
