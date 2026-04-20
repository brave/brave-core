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
      .labelStyle(QuickViewToolbarLabelIconStyle())
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
          Label {
            Text(Strings.quickViewPlaylistAccessibilityLabel)
          } icon: {
            Image(braveSystemName: "leo.product.playlist-add")
              .font(.headline)
          }
          .labelStyle(QuickViewToolbarLabelIconStyle())
        }
      case .readerMode:
        Button {
          viewModel.onActionButton?(.readerMode)
        } label: {
          Label {
            Text(Strings.quickViewReaderModeAccessibilityLabel)
          } icon: {
            Image(braveSystemName: "leo.product.speedreader")
              .font(.headline)
          }
          .labelStyle(QuickViewToolbarLabelIconStyle())
        }
      case .translate:
        Button {
          viewModel.onActionButton?(.translate)
        } label: {
          Label {
            Text(Strings.quickViewTranslateAccessibilityLabel)
          } icon: {
            Image(braveSystemName: "leo.product.translate")
              .font(.headline)
          }
          .labelStyle(QuickViewToolbarLabelIconStyle())
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
      Label {
        Text(Strings.quickViewRefreshAccessibilityLabel)
      } icon: {
        Image(braveSystemName: "leo.browser.refresh")
          .font(.headline)
      }
      .labelStyle(QuickViewToolbarLabelIconStyle())
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
  }

  private var backButton: some View {
    Button {
      viewModel.onActionButton?(.back)
    } label: {
      Label {
        Text(Strings.quickViewBackAccessibilityLabel)
      } icon: {
        Image(braveSystemName: "leo.browser.back")
          .font(.title2)
      }
      .labelStyle(QuickViewToolbarLabelIconStyle())
    }
    .disabled(viewModel.isBackDisabled)
  }

  private var shareButton: some View {
    Button {
      viewModel.onActionButton?(.share)
    } label: {
      Label {
        Text(Strings.quickViewShareAccessibilityLabel)
      } icon: {
        Image(braveSystemName: "leo.share.macos")
          .font(.title2)
      }
      .labelStyle(QuickViewToolbarLabelIconStyle())
    }
  }

  private var openInTabButton: some View {
    Button {
      viewModel.onActionButton?(.openTab)
    } label: {
      Label {
        Text(Strings.quickViewOpenTabAccessibilityLabel)
      } icon: {
        Image(braveSystemName: "leo.add.tab")
          .font(.title2)
      }
      .labelStyle(QuickViewToolbarLabelIconStyle())
    }
  }

  private var closeButton: some View {
    Button {
      viewModel.onActionButton?(.close)
    } label: {
      Label {
        Text(Strings.close)
      } icon: {
        Image(braveSystemName: "leo.close")
          .font(.title2)
      }
      .labelStyle(QuickViewToolbarLabelIconStyle())
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
  }
}

private struct QuickViewToolbarLabelIconStyle: LabelStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.icon
      .tint(Color(braveSystemName: .iconDefault))
      .accessibilityRepresentation {
        configuration.title
      }
  }
}
