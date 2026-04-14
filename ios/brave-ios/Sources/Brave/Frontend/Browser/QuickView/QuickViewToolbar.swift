// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Strings
import SwiftUI
import UIKit

enum QuickViewActionButton {
  case shield
  case playlist
  case readerMode
  case translate
  case refresh
  case close
  case back
  case share
  case openTab
}

@Observable
class QuickViewToolbarModel {
  var url: URL
  var secondaryTopButton: QuickViewActionButton?
  var isBackDisabled: Bool = true
  var onActionButton: ((QuickViewActionButton) -> Void)?

  init(
    url: URL,
    secondaryTopButton: QuickViewActionButton? = nil,
    onActionButton: ((QuickViewActionButton) -> Void)? = nil
  ) {
    self.url = url
    self.secondaryTopButton = secondaryTopButton
    self.onActionButton = onActionButton
  }
}

struct QuickViewToolbarView: View {
  var viewModel: QuickViewToolbarModel

  var body: some View {
    VStack(spacing: 0) {
      topRow
        .padding(.horizontal, 16)
        .padding(.top, 8)
      bottomRow
        .padding(.leading, 16)
        .padding(.top, 16)
    }
    .padding(8)
    .osAvailabilityModifiers { content in
      if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
        content
          .background(
            Color.clear
              .glassEffect(in: RoundedRectangle(cornerRadius: 30, style: .continuous))
          )
      } else {
        content
          .background(
            .ultraThinMaterial,
            in: .rect(cornerRadius: 30, style: .continuous)
          )
      }
    }
    .padding(8)
  }

  private var shieldButton: some View {
    Button {
      viewModel.onActionButton?(.shield)
    } label: {
      Label(
        Strings.quickViewShieldAccessibilityLabel,
        braveSystemImage: "leo.shield.done"
      )
      .labelStyle(QuickViewToolbarLabelIconStyle())
    }
    .accessibilityLabel(Strings.quickViewShieldAccessibilityLabel)
  }

  @ViewBuilder
  private var secondaryTopButtonView: some View {
    if let button = viewModel.secondaryTopButton {
      if button == .playlist {
        Button {
          viewModel.onActionButton?(.playlist)
        } label: {
          Label(
            Strings.quickViewPlaylistAccessibilityLabel,
            braveSystemImage: "leo.product.playlist-add"
          )
          .labelStyle(QuickViewToolbarLabelIconStyle())
        }
        .accessibilityLabel(Strings.quickViewPlaylistAccessibilityLabel)
      } else if button == .readerMode {
        Button {
          viewModel.onActionButton?(.readerMode)
        } label: {
          Label(
            Strings.quickViewReaderModeAccessibilityLabel,
            braveSystemImage: "leo.product.speedreader"
          )
          .labelStyle(QuickViewToolbarLabelIconStyle())
        }
        .accessibilityLabel(Strings.quickViewReaderModeAccessibilityLabel)
      } else {
        Button {
          viewModel.onActionButton?(.translate)
        } label: {
          Label(
            Strings.quickViewTranslateAccessibilityLabel,
            braveSystemImage: "leo.product.translate"
          )
          .labelStyle(QuickViewToolbarLabelIconStyle())
        }
        .accessibilityLabel(Strings.quickViewTranslateAccessibilityLabel)
      }
    }
  }

  private var refreshButton: some View {
    Button {
      viewModel.onActionButton?(.refresh)
    } label: {
      Label(
        Strings.quickViewRefreshAccessibilityLabel,
        braveSystemImage: "leo.browser.refresh"
      )
      .labelStyle(QuickViewToolbarLabelIconStyle())
    }
    .accessibilityLabel(Strings.quickViewRefreshAccessibilityLabel)
  }

  private var addressView: some View {
    HStack(spacing: 0) {
      Spacer()
      Text(viewModel.url.host ?? viewModel.url.absoluteString)
        .font(.subheadline)
        .foregroundColor(Color(braveSystemName: .textTertiary))
        .lineLimit(1)
        .frame(maxWidth: .infinity)
        .accessibilityLabel(viewModel.url.host ?? viewModel.url.absoluteString)
      Spacer()
    }
  }

  private var topLeftButtonsView: some View {
    HStack(spacing: 8) {
      shieldButton
      secondaryTopButtonView
    }
  }

  private var topRow: some View {
    HStack(spacing: 8) {
      topLeftButtonsView

      addressView

      refreshButton
    }
  }

  private var backButton: some View {
    Button {
      viewModel.onActionButton?(.back)
    } label: {
      Label(
        Strings.quickViewBackAccessibilityLabel,
        braveSystemImage: "leo.browser.back"
      )
      .labelStyle(QuickViewToolbarLabelIconStyle(font: .title2))
    }
    .disabled(viewModel.isBackDisabled)
    .accessibilityLabel(Strings.quickViewBackAccessibilityLabel)
  }

  private var shareButton: some View {
    Button {
      viewModel.onActionButton?(.share)
    } label: {
      Label(
        Strings.quickViewShareAccessibilityLabel,
        braveSystemImage: "leo.share.macos"
      )
      .labelStyle(QuickViewToolbarLabelIconStyle(font: .title2))
    }
    .accessibilityLabel(Strings.quickViewShareAccessibilityLabel)
  }

  private var openInTabButton: some View {
    Button {
      viewModel.onActionButton?(.openTab)
    } label: {
      Label(
        Strings.quickViewOpenTabAccessibilityLabel,
        braveSystemImage: "leo.add.tab"
      )
      .labelStyle(QuickViewToolbarLabelIconStyle(font: .title2))
    }
    .accessibilityLabel(Strings.quickViewOpenTabAccessibilityLabel)
  }

  private var closeButton: some View {
    Button {
      viewModel.onActionButton?(.close)
    } label: {
      if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
        Image(braveSystemName: "leo.close")
          .font(.title2)
          .padding(14)
          .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
          .glassEffect(
            .regular
              .tint(Color(braveSystemName: .buttonBackground))
              .interactive(),
            in: .circle
          )
      } else {
        Image(braveSystemName: "leo.close")
          .font(.title2)
          .padding(14)
          .foregroundColor(Color(braveSystemName: .schemesOnPrimary))
          .background(
            Color(braveSystemName: .buttonBackground),
            in: .circle
          )
      }
    }
    .accessibilityLabel(Strings.close)
  }

  private var bottomRow: some View {
    HStack(spacing: 0) {
      backButton

      Spacer()

      shareButton

      Spacer()

      openInTabButton

      Spacer()

      closeButton
    }
  }
}

private struct QuickViewToolbarLabelIconStyle: LabelStyle {
  var font: Font = .headline
  var disabled: Bool = false

  func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .font(font)
      .tint(disabled ? Color(braveSystemName: .iconDisabled) : Color(braveSystemName: .iconDefault))
  }
}
