// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import BraveCore
import Strings
import DesignSystem
import Preferences
import StoreKit

public struct AIChatAdvancedSettingsView: View {
  @Environment(\.dismiss)
  private var dismiss
  
  @ObservedObject
  private var viewModel: AIChatSubscriptionDetailModelView
  
  @State 
  private var appStoreConnectionErrorPresented = false

  @State
  private var resetAndClearAlertErrorPresented = false
  
  @State
  private var isPaywallPresented = false

  var isModallyPresented: Bool
  
  var openURL: ((URL) -> Void)
  
  public init(viewModel: AIChatSubscriptionDetailModelView, isModallyPresented: Bool, openURL: @escaping (URL) -> Void) {
    self.viewModel = viewModel
    self.isModallyPresented = isModallyPresented
    self.openURL = openURL
  }

  public var body: some View {
    if isModallyPresented {
      NavigationView {
        settingsView
          .navigationTitle(Strings.AIChat.leoNavigationTitle)
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
          ToolbarItemGroup(placement: .cancellationAction) {
            Button(Strings.close) {
              dismiss()
            }
          }
        }
      }
      .navigationViewStyle(.stack)
    } else {
      settingsView
        .navigationTitle(Strings.AIChat.leoNavigationTitle)
        .navigationBarTitleDisplayMode(.inline)
    }
  }
  
  private var subscriptionMenuTitle: String {
    // Display the info from the AppStore
    if let state = viewModel.inAppPurchaseSubscriptionState {
      switch state {
      case .subscribed, .inBillingRetryPeriod, .inGracePeriod:
        return Strings.AIChat.manageSubscriptionsButtonTitle
      case .expired, .revoked:
        return Strings.AIChat.goPremiumButtonTitle
      default:
        return Strings.AIChat.goPremiumButtonTitle
      }
    }
    
    // Display the info from SkusSDK
    if viewModel.skuOrderStatus == .active {
      return Strings.AIChat.manageSubscriptionsButtonTitle
    }
    
    // No order found
    return Strings.AIChat.goPremiumButtonTitle
  }
  
  private var subscriptionStatusTitle: String {
    // Display the info from the AppStore
    let inAppPurchaseProductType = viewModel.inAppPurchasedProductType
    
    switch inAppPurchaseProductType {
    case .leoMonthly:
      return Strings.AIChat.monthlySubscriptionTitle
    case .leoYearly:
      return Strings.AIChat.yearlySubscriptionTitle
    default:
      break
    }

    // Display the info from SkusSDK
    let skuProductType = viewModel.skuOrderProductType
    
    switch skuProductType {
    case .monthly:
      return Strings.AIChat.monthlySubscriptionTitle
    case .yearly:
      return Strings.AIChat.yearlySubscriptionTitle
    case .notDetermined:
      return Strings.AIChat.premiumSubscriptionTitle
    default:
      break
    }

    // No order found
    return "None"
  }
  
  private var expirationDateTitle: String {
    let dateFormatter = DateFormatter().then {
      $0.locale = Locale.current
    }
    
    let periodToDate = { (subscription: StoreKit.Product.SubscriptionPeriod) -> Date? in
      let now = Date.now
      if subscription.value == 0 {
        return now
      }
      
      switch subscription.unit {
      case .day:
        return Calendar.current.date(byAdding: .day, value: subscription.value, to: now)
      case .week:
        return Calendar.current.date(byAdding: .weekOfYear, value: subscription.value, to: now)
      case .month:
        return Calendar.current.date(byAdding: .month, value: subscription.value, to: now)
      case .year:
        return Calendar.current.date(byAdding: .year, value: subscription.value, to: now)
      @unknown default:
        return nil
      }
    }
    
    if let period = viewModel.inAppPurchaseSubscriptionPeriod,
       let date = periodToDate(period) {
      return dateFormatter.string(from: date)
    }
    
    // Display the info from SkusSDK
    if let expiryDate = viewModel.skuOrderExpirationDate {
      return dateFormatter.string(from: expiryDate)
    }
    
    return Strings.AIChat.leoSubscriptionUnknownDateTitle
  }
  
  private var settingsView: some View {
    List {
      Section {
        OptionToggleView(
          title: Strings.AIChat.advancedSettingsAutocompleteTitle,
          option: Preferences.AIChat.autocompleteSuggestionsEnabled
        )
        
        NavigationLink {
          AIChatDefaultModelView(aiModel: viewModel.model)
        } label: {
          LabelView(
            title: Strings.AIChat.advancedSettingsDefaultModelTitle,
            subtitle: viewModel.model.currentModel.displayName
          )
        }.listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        Text(Strings.AIChat.advancedSettingsHeaderTitle)
          .textCase(nil)
      }
      
      Section {
        if viewModel.canDisplaySubscriptionStatus {
          if viewModel.isSubscriptionStatusLoading {
            LabelDetailView(title: Strings.AIChat.advancedSettingsSubscriptionStatusTitle,
                            detail: subscriptionStatusTitle)
            
            LabelDetailView(title: Strings.AIChat.advancedSettingsSubscriptionExpiresTitle,
                            detail: expirationDateTitle)
          } else {
            // Subscription information is loading
            LabelDetailView(title: Strings.AIChat.advancedSettingsSubscriptionStatusTitle,
                            detail: nil)
            
            LabelDetailView(title: Strings.AIChat.advancedSettingsSubscriptionExpiresTitle,
                            detail: nil)
          }
          
          // Check subscription is activated with in-app purchase
          if viewModel.canSubscriptionBeLinked {
            Button(action: {
              openURL(.brave.braveLeoLinkReceiptProd)
            }) {
              LabelView(
                title: Strings.AIChat.advancedSettingsLinkPurchaseActionTitle,
                subtitle: Strings.AIChat.advancedSettingsLinkPurchaseActionSubTitle
              )
            }
            
            if viewModel.isDevReceiptLinkingAvailable {
              Button(action: {
                openURL(.brave.braveLeoLinkReceiptStaging)
              }) {
                LabelView(
                  title: "[Staging] Link receipt"
                )
              }
              
              Button(action: {
                openURL(.brave.braveLeoLinkReceiptDev)
              }) {
                LabelView(
                  title: "[Dev] Link receipt"
                )
              }
            }
            
            Button(action: {
              guard let url = URL.apple.manageSubscriptions else {
                return
              }
              
              // Opens Apple's 'manage subscription' screen
              if UIApplication.shared.canOpenURL(url) {
                UIApplication.shared.open(url, options: [:])
              }
            }) {
              premiumActionView
            }
          }
        } else {
          Button(action: {
            if viewModel.inAppPurchaseProductsLoaded {
              isPaywallPresented = true
            } else {
              appStoreConnectionErrorPresented = true
            }
          }) {
            premiumActionView
          }
        }
      } header: {
        Text(Strings.AIChat.advancedSettingsSubscriptionHeaderTitle)
      }
      .sheet(isPresented: $isPaywallPresented) {
        AIChatPaywallView()
      }
      .alert(isPresented: $appStoreConnectionErrorPresented) {
        Alert(title: Text(Strings.AIChat.appStoreErrorTitle),
              message: Text(Strings.AIChat.appStoreErrorSubTitle),
              dismissButton: .default(Text(Strings.OKString)))
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      
      Section {
        Button(action: {
          resetAndClearAlertErrorPresented.toggle()
        }) {
          Text(Strings.AIChat.resetLeoDataActionTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        .frame(maxWidth: .infinity)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .alert(isPresented: $resetAndClearAlertErrorPresented) {
        Alert(
          title: Text(Strings.AIChat.resetLeoDataErrorTitle),
          message: Text(Strings.AIChat.resetLeoDataErrorDescription),
          primaryButton: .destructive(Text(Strings.AIChat.resetLeoDataAlertButtonTitle)) {
            viewModel.model.clearAndResetData()
          },
          secondaryButton: .cancel())
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .listStyle(.insetGrouped)
  }

  var premiumActionView: some View {
    HStack {
      LabelView(title: subscriptionMenuTitle)
      Spacer()
      Image(braveSystemName: "leo.launch")
        .foregroundStyle(Color(braveSystemName: .iconDefault))
    }
  }
}
