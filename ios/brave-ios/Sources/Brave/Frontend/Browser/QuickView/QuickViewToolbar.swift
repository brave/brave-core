// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import SwiftUI
import UIKit

enum QuickViewSecondaryTopButton {
  case playlist(action: () -> Void)
  case readerMode(action: () -> Void)
  case translate(action: () -> Void)
}

@Observable
class QuickViewToolbarModel {
  var url: URL
  var secondaryTopButton: QuickViewSecondaryTopButton?
  var isBackDisabled: Bool = true
  var onShield: (() -> Void)?
  var onRefresh: (() -> Void)?
  var onClose: (() -> Void)?
  var onBack: (() -> Void)?
  var onShare: (() -> Void)?
  var onOpenTab: (() -> Void)?

  init(
    url: URL,
    secondaryTopButton: QuickViewSecondaryTopButton? = nil,
    onClose: (() -> Void)? = nil
  ) {
    self.url = url
    self.secondaryTopButton = secondaryTopButton
    self.onClose = onClose
  }
}

struct QuickViewToolbarView: View {
  var viewModel: QuickViewToolbarModel

  var body: some View {
    toolbarContent
      .padding(8)
  }

  private var toolbarContent: some View {
    VStack(spacing: 0) {
      topRow
        .padding(.horizontal, 16)
        .padding(.top, 8)
      bottomRow
        .padding(.leading, 16)
        .padding(.top, 16)
    }
    .padding(8)
    .background(
      Group {
        if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
          Color.clear
            .glassEffect(in: RoundedRectangle(cornerRadius: 30, style: .continuous))
        } else {
          RoundedRectangle(cornerRadius: 30, style: .continuous)
            .fill(.ultraThinMaterial)
        }
      }
    )
  }

  private var shieldButton: some View {
    Button {
      viewModel.onShield?()
    } label: {
      Image(braveSystemName: "leo.shield.done")
        .font(.headline)
        .foregroundColor(Color(braveSystemName: .iconDefault))
    }
  }

  @ViewBuilder
  private var secondaryTopButtonView: some View {
    if let button = viewModel.secondaryTopButton {
      switch button {
      case .playlist(let action):
        Button(action: action) {
          Image(braveSystemName: "leo.product.playlist-add")
            .font(.headline)
            .foregroundColor(Color(braveSystemName: .iconDefault))
        }
      case .readerMode(let action):
        Button(action: action) {
          Image(braveSystemName: "leo.product.speedreader")
            .font(.headline)
            .foregroundColor(Color(braveSystemName: .iconDefault))
        }
      case .translate(let action):
        Button(action: action) {
          Image(braveSystemName: "leo.product.translate")
            .font(.headline)
            .foregroundColor(Color(braveSystemName: .iconDefault))
        }
      }
    }
  }

  private var refreshButton: some View {
    Button {
      viewModel.onRefresh?()
    } label: {
      Image(braveSystemName: "leo.browser.refresh")
        .font(.headline)
        .foregroundColor(Color(braveSystemName: .iconDefault))
    }
  }

  private var addressView: some View {
    HStack(spacing: 0) {
      Spacer()
      Text(viewModel.url.host ?? viewModel.url.absoluteString)
        .font(.subheadline)
        .foregroundColor(Color(braveSystemName: .textTertiary))
        .lineLimit(1)
        .frame(maxWidth: .infinity)
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
      viewModel.onBack?()
    } label: {
      Image(braveSystemName: "leo.browser.back")
        .font(.title2)
        .foregroundColor(
          viewModel.isBackDisabled
            ? Color(braveSystemName: .iconDisabled) : Color(braveSystemName: .iconDefault)
        )
    }
    .disabled(viewModel.isBackDisabled)
  }

  private var shareButton: some View {
    Button {
      viewModel.onShare?()
    } label: {
      Image(braveSystemName: "leo.share.macos")
        .font(.title2)
        .foregroundColor(Color(braveSystemName: .iconDefault))
    }
  }

  private var openInTabButton: some View {
    Button {
      viewModel.onOpenTab?()
    } label: {
      Image(braveSystemName: "leo.add.tab")
        .font(.title2)
        .foregroundColor(Color(braveSystemName: .iconDefault))
    }
  }

  private var closeButton: some View {
    Button {
      viewModel.onClose?()
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
