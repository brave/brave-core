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
  @State private var isNewCustomModelFormPresented: Bool = false

  public init(viewModel: AIChatSettingsViewModel) {
    self.viewModel = viewModel
  }

  public var body: some View {
    Form {
      Section {
        Toggle(isOn: $leoInQuickSearchBarEnabled.value) {
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.AIChat.advancedSettingsShowInQSEBarTitle)
            Text(LocalizedStringKey(Strings.AIChat.advancedSettingsShowInQSEBarDescription))
              .foregroundStyle(.secondary)
              .font(.caption)
          }
          .tint(Color(braveSystemName: .primitivePrimary40))
        }
        NavigationLink {
          ModelListPicker(
            modelsWithSubtitles: viewModel.modelsWithSubtitles,
            selectedModel: $viewModel.defaultModelWithSubtitle
          )
        } label: {
          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.AIChat.advancedSettingsDefaultModelTitle)
            if let defaultModelWithSubtitle = viewModel.defaultModelWithSubtitle {
              Text(defaultModelWithSubtitle.model.displayName)
                .foregroundStyle(.secondary)
                .font(.caption)
            }
          }
        }
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

      Section {
        if viewModel.customModels.isEmpty {
          VStack(alignment: .leading) {
            Text(Strings.AIChat.byomEmptyStateTitle)
            Text(Strings.AIChat.byomEmptyStateDescription)
              .font(.subheadline.weight(.regular))
              .foregroundStyle(.secondary)
          }
        }
        ForEach(viewModel.customModels, id: \.key) { model in
          NavigationLink {
            CustomModelForm(initialModel: model, helper: viewModel.helper)
          } label: {
            VStack(alignment: .leading) {
              Text(model.displayName)
              if let requestName = model.options.customModelOptions?.modelRequestName {
                Text(requestName)
                  .font(.subheadline.weight(.regular))
                  .foregroundStyle(.secondary)
              }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
          }
        }
        .onDelete { indexSet in
          guard let index = indexSet.first else { return }
          withAnimation {
            viewModel.helper.deleteCustomModel(at: index)
          }
        }
        Button {
          isNewCustomModelFormPresented = true
        } label: {
          Text(Strings.AIChat.byomAddNewModelButtonTitle)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
        }
        .sheet(isPresented: $isNewCustomModelFormPresented) {
          NavigationStack {
            CustomModelForm(helper: viewModel.helper)
          }
        }
      } header: {
        VStack(alignment: .leading) {
          Text(Strings.AIChat.byomSectionHeaderTitle)
          // Contains markdown
          Text(LocalizedStringKey(Strings.AIChat.byomSectionHeaderDescription))
            .textCase(.none)
            .tint(Color(braveSystemName: .textInteractive))
        }
      }
    }
    .navigationTitle(Strings.AIChat.leoNavigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .animation(.default, value: viewModel.premiumStatus)
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
                  Group {
                    if let customModelOptions = model.options.customModelOptions {
                      Text(customModelOptions.modelRequestName)
                    } else {
                      Text(modelWithSubtitle.subtitle)
                    }
                  }
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
        }
      }
      .navigationTitle(Strings.AIChat.advancedSettingsDefaultModelTitle)
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
          if let expirationDate = activePremiumDetails.expirationDate {
            LabeledContent(
              Strings.AIChat.advancedSettingsSubscriptionExpiresTitle,
              value: expirationDate.formatted(date: .numeric, time: .omitted)
            )
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
              Button {
                openURL(.brave.braveLeoLinkReceiptDev)
              } label: {
                LabelView(
                  title: "[Dev] Link receipt"
                )
                .contentShape(Rectangle())
              }
              .buttonStyle(.plain)
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
          }
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
        Spacer()
        configuration.content
          .foregroundStyle(.secondary)
      }
    }
  }
}
