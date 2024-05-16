// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import SwiftUI

struct AIChatDefaultModelView: View {

  @Environment(\.dismiss)
  private var dismiss

  @State
  private var isPresentingPaywallPremium: Bool = false

  @ObservedObject
  var aiModel: AIChatViewModel

  var premiumStatus: String {
    switch aiModel.premiumStatus {
    case .active, .activeDisconnected:
      return Strings.AIChat.unlimitedModelStatusTitle.uppercased()
    default:
      return Strings.AIChat.limitedModelStatusTitle.uppercased()
    }
  }

  var body: some View {
    List {
      Section {
        ForEach(aiModel.models, id: \.key) { model in
          Button(
            action: {
              if model.access == .premium, aiModel.shouldShowPremiumPrompt {
                isPresentingPaywallPremium = true
              } else {
                aiModel.defaultAIModelKey = model.key
                dismiss()
              }
            },
            label: {
              HStack(spacing: 0.0) {
                VStack(alignment: .leading) {
                  Text(model.displayName)
                    .foregroundStyle(Color(braveSystemName: .textPrimary))

                  Text(model.displayMaker)
                    .font(.footnote)
                    .foregroundStyle(Color(braveSystemName: .textSecondary))
                }
                .frame(maxWidth: .infinity, alignment: .leading)

                // If the model is selected show check
                if model.key == aiModel.defaultAIModelKey {
                  Image(braveSystemName: "leo.check.normal")
                    .foregroundStyle(Color(braveSystemName: .textInteractive))
                    .padding(.horizontal, 4.0)
                } else {
                  if model.access == .basicAndPremium {
                    Text(premiumStatus)
                      .font(.caption2)
                      .foregroundStyle(Color(braveSystemName: .blue50))
                      .padding(.horizontal, 4.0)
                      .padding(.vertical, 2.0)
                      .overlay(
                        RoundedRectangle(cornerRadius: 4.0, style: .continuous)
                          .strokeBorder(Color(braveSystemName: .blue50), lineWidth: 1.0)
                      )
                  } else if model.access == .premium {
                    Image(
                      braveSystemName: aiModel.premiumStatus != .active
                        && aiModel.premiumStatus != .activeDisconnected
                        ? "leo.lock.plain" : "leo.lock.open"
                    )
                    .foregroundStyle(Color(braveSystemName: .iconDefault))
                    .padding(.horizontal, 4.0)
                  }
                }
              }
              .contentShape(Rectangle())
            }
          )
          .buttonStyle(.plain)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.AIChat.defaultModelChatSectionTitle.uppercased())
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .listStyle(.insetGrouped)
    .navigationTitle(Strings.AIChat.defaultModelViewTitle)
    .sheet(isPresented: $isPresentingPaywallPremium) {
      AIChatPaywallView(
        premiumUpgrageSuccessful: { _ in
          Task { @MainActor in
            await aiModel.refreshPremiumStatus()
          }
        })
    }
  }
}
