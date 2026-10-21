// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Strings
import SwiftUI

/// Shown the first time a link is opened from another app, offering to turn on `Ask` mode.
public struct ExternalLinkOnboardingView: View {
  var onEnable: () -> Void
  var onDecline: () -> Void

  public init(onEnable: @escaping () -> Void, onDecline: @escaping () -> Void) {
    self.onEnable = onEnable
    self.onDecline = onDecline
  }

  public var body: some View {
    Form {
      Section {
        VStack(spacing: 12) {
          Image(braveSystemName: "leo.window.tab-new")
            .font(.largeTitle)
            .foregroundStyle(Color(braveSystemName: .iconInteractive))
          Text(Strings.ExternalLinks.onboardingTitle)
            .font(.title3.weight(.semibold))
          Text(
            FeatureList.kQuickViewEnabled.enabled
              ? Strings.ExternalLinks.onboardingMessage
              : Strings.ExternalLinks.onboardingMessageQuickViewDisabled
          )
          .font(.subheadline)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        .multilineTextAlignment(.center)
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
        .listRowBackground(Color.clear)
      }
      Section {
        Button(action: onEnable) {
          Text(Strings.ExternalLinks.onboardingEnableButtonTitle)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(.filled)
        .listRowInsets(.init())
        .listRowBackground(Color.clear)
        Button(action: onDecline) {
          Text(Strings.ExternalLinks.onboardingDeclineButtonTitle)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(.plain)
        .foregroundStyle(Color(braveSystemName: .textInteractive))
        .listRowBackground(Color.clear)
      }
    }
    .listSectionSpacing(.compact)
    .contentMargins(.top, 16, for: .scrollContent)
    .overlay(alignment: .topLeading) {
      Button(action: onDecline) {
        Image(braveSystemName: "leo.close")
          .foregroundStyle(Color(braveSystemName: .iconDefault))
          .frame(width: 44, height: 44)
      }
      .accessibilityLabel(Strings.close)
      .padding(8)
    }
    .reportsSheetContentHeight()
  }
}

#if DEBUG
#Preview {
  Color.clear
    .sheet(isPresented: .constant(true)) {
      ExternalLinkOnboardingView(onEnable: {}, onDecline: {})
        .presentationDetents([.medium])
    }
}
#endif
