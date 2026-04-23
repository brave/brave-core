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
  var isShieldDisabled: Bool = false
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
  let viewModel: QuickViewToolbarModel

  var body: some View {
    VStack(spacing: 0) {
      topRow
      bottomRow
        .padding(.top, 16)
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
        viewModel.isShieldDisabled
          ? Image(sharedName: "brave.logo.greyscale") : Image(sharedName: "brave.logo")
      }
    }
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
        }
      case .readerMode:
        Button {
          viewModel.onActionButton?(.readerMode)
        } label: {
          Label(
            Strings.quickViewReaderModeAccessibilityLabel,
            braveSystemImage: "leo.product.speedreader"
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
    }
  }

  private var addressView: some View {
    Text(viewModel.url.host ?? viewModel.url.absoluteString)
      .font(.subheadline)
      .foregroundStyle(Color(braveSystemName: .textTertiary))
      .lineLimit(1)
      .frame(maxWidth: .infinity)
      .accessibilityLabel(viewModel.url.host ?? viewModel.url.absoluteString)
  }

  private var topRightButtonsView: some View {
    HStack(spacing: 12) {
      secondaryTopButtonView
      refreshButton
    }
  }

  private var topRow: some View {
    HStack(spacing: 8) {
      shieldButton

      addressView

      topRightButtonsView
    }
    .labelStyle(QuickViewToolbarLabelTopIconStyle())
  }

  private var backButton: some View {
    Button {
      viewModel.onActionButton?(.back)
    } label: {
      Label(Strings.quickViewBackAccessibilityLabel, braveSystemImage: "leo.browser.back")
    }
    .disabled(viewModel.isBackDisabled)
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
      Label(Strings.quickViewOpenTabAccessibilityLabel, braveSystemImage: "leo.add.tab")
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

      shareButton

      Spacer()

      openInTabButton

      Spacer()

      closeButton
    }
    .labelStyle(QuickViewToolbarLabelBottomIconStyle())
  }
}

private struct QuickViewToolbarLabelTopIconStyle: LabelStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .font(.headline)
      .tint(Color(braveSystemName: .iconDefault))
      .accessibilityRepresentation {
        configuration.title
      }
  }
}

private struct QuickViewToolbarLabelBottomIconStyle: LabelStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .font(.title2)
      .tint(Color(braveSystemName: .iconDefault))
      .accessibilityRepresentation {
        configuration.title
      }
  }
}
