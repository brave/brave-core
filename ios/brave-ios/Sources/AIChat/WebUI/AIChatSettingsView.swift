// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import BraveUI
import DesignSystem
import Preferences
import SwiftUI

/// Displays settings associated with AI Chat
public struct AIChatSettingsView: View {
  @Bindable var viewModel: AIChatSettingsViewModel

  @ObservedObject private var leoInQuickSearchBarEnabled = Preferences.AIChat
    .leoInQuickSearchBarEnabled

  @State private var isResetConfirmationDialogPresented: Bool = false
  @State private var isPaywallPresented: Bool = false

  public init(viewModel: AIChatSettingsViewModel) {
    self.viewModel = viewModel
  }

  public var body: some View {
    Form {
      Section {
        Toggle(isOn: $leoInQuickSearchBarEnabled.value) {
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.AIChat.advancedSettingsShowInQSEBarTitle)
              .foregroundColor(Color(.bravePrimary))
            Text(LocalizedStringKey(Strings.AIChat.advancedSettingsShowInQSEBarDescription))
              .foregroundColor(Color(.braveLabel))
              .font(.caption)
          }
        }
        .tint(.accentColor)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        NavigationLink {
          ModelListPicker(
            modelsWithSubtitles: viewModel.modelsWithSubtitles,
            selectedModel: $viewModel.defaultModelWithSubtitle
          )
        } label: {
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.AIChat.advancedSettingsDefaultModelTitle)
              .foregroundColor(Color(.bravePrimary))
            if let defaultModelWithSubtitle = viewModel.defaultModelWithSubtitle {
              Text(defaultModelWithSubtitle.model.displayName)
                .foregroundColor(Color(.braveLabel))
                .font(.caption)
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        Text(Strings.AIChat.advancedSettingsHeaderTitle)
          .textCase(nil)
      }

      SubscriptionSummarySection(viewModel: viewModel)

      Section {
        Button(role: .destructive) {
          isResetConfirmationDialogPresented = true
        } label: {
          Text(Strings.AIChat.resetLeoDataActionTitle)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .confirmationDialog(
          Strings.AIChat.resetLeoDataErrorTitle,
          isPresented: $isResetConfirmationDialogPresented
        ) {
          Button(Strings.AIChat.resetLeoDataAlertButtonTitle, role: .destructive) {
            viewModel.resetData()
          }
        } message: {
          Text(Strings.AIChat.resetLeoDataErrorDescription)
        }
      }
    }
    .navigationTitle(Strings.AIChat.leoNavigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .animation(.default, value: viewModel.premiumStatus)
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
  }

  private struct ModelListPicker: View {
    var modelsWithSubtitles: [AiChat.ModelWithSubtitle]
    @Binding var selectedModel: AiChat.ModelWithSubtitle?

    @Environment(\.dismiss) private var dismiss

    var body: some View {
      Form {
        ForEach(modelsWithSubtitles, id: \.model.key) { modelWithSubtitle in
          Button {
            selectedModel = modelWithSubtitle
            dismiss()
          } label: {
            let model = modelWithSubtitle.model
            Label {
              HStack {
                VStack(alignment: .leading) {
                  Text(model.displayName)
                    .foregroundStyle(Color(braveSystemName: .textPrimary))
                  Text(modelWithSubtitle.subtitle)
                    .foregroundStyle(Color(braveSystemName: .textTertiary))
                    .font(.callout)
                }
                Spacer()
                if model.options.leoModelOptions?.access == .premium {
                  Text(Strings.AIChat.premiumModelStatusTitle)
                    .foregroundStyle(Color(braveSystemName: .primary40))
                    .font(.caption.weight(.semibold))
                    .padding(4)
                    .background(
                      Color(braveSystemName: .primary40),
                      in: .rect(cornerRadius: 4).stroke(lineWidth: 1)
                    )
                }
              }
            } icon: {
              if selectedModel?.model.key == modelWithSubtitle.model.key {
                Image(braveSystemName: "leo.check.normal")
                  .foregroundStyle(Color(braveSystemName: .primary40))
              } else {
                Color.clear
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      .navigationTitle(Strings.AIChat.advancedSettingsDefaultModelTitle)
      .scrollContentBackground(.hidden)
      .background(Color(.braveGroupedBackground))
    }
  }

  private struct SubscriptionSummarySection: View {
    var viewModel: AIChatSettingsViewModel

    @State private var isPaywallPresented: Bool = false
    @State private var isAppStoreConnectionErrorPresented: Bool = false

    @Environment(\.openURL) private var openURL
    @Environment(\.dismiss) private var dismiss

    var body: some View {
      Section {
        switch viewModel.premiumStatus {
        case .unknown:
          ProgressView()
            .progressViewStyle(.circular)
        case .active(let activePremiumDetails):
          LabeledContent(
            Strings.AIChat.advancedSettingsSubscriptionStatusTitle,
            value: {
              switch activePremiumDetails.plan {
              case .none:
                return Strings.AIChat.premiumSubscriptionTitle
              case .monthly:
                return Strings.AIChat.monthlySubscriptionTitle
              case .yearly:
                return Strings.AIChat.yearlySubscriptionTitle
              }
            }()
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))

          if let expirationDate = activePremiumDetails.expirationDate {
            LabeledContent(
              Strings.AIChat.advancedSettingsSubscriptionExpiresTitle,
              value: expirationDate.formatted(date: .numeric, time: .omitted)
            )
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          if activePremiumDetails.isSubscriptionLinkable {
            Button {
              openURL(.brave.braveLeoLinkReceiptProd)
            } label: {
              LabelView(
                title: Strings.AIChat.advancedSettingsLinkPurchaseActionTitle,
                subtitle: Strings.AIChat.advancedSettingsLinkPurchaseActionSubTitle
              )
              .contentShape(Rectangle())
            }
            .buttonStyle(.plain)
            .listRowBackground(Color(.secondaryBraveGroupedBackground))

            if viewModel.isDevReceiptLinkingAvailable {
              Button {
                openURL(.brave.braveLeoLinkReceiptStaging)
              } label: {
                LabelView(
                  title: "[Staging] Link receipt"
                )
                .contentShape(Rectangle())
              }
              .buttonStyle(.plain)
              .listRowBackground(Color(.secondaryBraveGroupedBackground))

              Button {
                openURL(.brave.braveLeoLinkReceiptDev)
              } label: {
                LabelView(
                  title: "[Dev] Link receipt"
                )
                .contentShape(Rectangle())
              }
              .buttonStyle(.plain)
              .listRowBackground(Color(.secondaryBraveGroupedBackground))
            }

            Button {
              // Opens Apple's 'manage subscription' screen
              if let url = URL.apple.manageSubscriptions, UIApplication.shared.canOpenURL(url) {
                UIApplication.shared.open(url, options: [:])
              }
            } label: {
              HStack {
                Text(Strings.AIChat.manageSubscriptionsButtonTitle)
                Spacer()
                Image(braveSystemName: "leo.launch")
                  .foregroundStyle(Color(braveSystemName: .iconDefault))
              }
              .contentShape(.rect)
            }
            .buttonStyle(.plain)
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        case .inactive(let productsLoaded):
          Button {
            if productsLoaded {
              isPaywallPresented = true
            } else {
              isAppStoreConnectionErrorPresented = true
            }
          } label: {
            HStack {
              Text(Strings.AIChat.goPremiumButtonTitle)
              Spacer()
              Image(braveSystemName: "leo.launch")
                .foregroundStyle(Color(braveSystemName: .iconDefault))
            }
            .contentShape(.rect)
          }
          .buttonStyle(.plain)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          .sheet(isPresented: $isPaywallPresented) {
            AIChatPaywallView(
              premiumUpgrageSuccessful: { _ in
                Task {
                  await viewModel.updatePremiumStatus()
                }
              },
              refreshCredentials: {
                openURL(.brave.braveLeoRefreshCredentials)
                dismiss()
              },
              openDirectCheckout: {
                openURL(.brave.braveLeoCheckoutURL)
                dismiss()
              }
            )
          }
          .alert(isPresented: $isAppStoreConnectionErrorPresented) {
            Alert(
              title: Text(Strings.AIChat.appStoreErrorTitle),
              message: Text(Strings.AIChat.appStoreErrorSubTitle),
              dismissButton: .default(Text(Strings.OKString))
            )
          }
        }

        // Check if there's an AppStore purchase
        if viewModel.isAppStoreReceiptAvailable {
          NavigationLink {
            StoreKitReceiptSimpleView()
          } label: {
            LabelView(title: Strings.AIChat.advancedSettingsViewReceiptTitle)
          }.listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.AIChat.advancedSettingsSubscriptionHeaderTitle.uppercased())
      }
      .labeledContentStyle(AIChatLabeledContentStyle())
    }
  }

  private struct AIChatLabeledContentStyle: LabeledContentStyle {
    func makeBody(configuration: Configuration) -> some View {
      HStack {
        configuration.label
          .foregroundStyle(Color(braveSystemName: .textPrimary))
        Spacer()
        configuration.content
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }
    }
  }
}
