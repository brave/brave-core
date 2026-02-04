// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStore
import BraveUI
import DesignSystem
import GuardianConnect
import OSLog
import Preferences
import Shared
import Static
import SwiftUI
import UIKit

@Observable
class VPNSettingsViewModel {
  let store: BraveStoreSDK
  private let skusService: (any SkusSkusService)?
  private let calendar: Calendar

  private var vpnStatusDidChangeObservable: NSObjectProtocol?

  var isVPNEnabled: Bool {
    get {
      access(keyPath: \.isVPNEnabled)
      return BraveVPN.isConnected || BraveVPN.isConnecting
    }
    set {
      if newValue {
        BraveVPN.reconnect { [weak self] _ in
          guard let self else { return }
          self.updateSelectedServer()
          self.withMutation(keyPath: \.isVPNEnabled) {}
        }
      } else {
        BraveVPN.disconnect { [weak self] _ in
          self?.withMutation(keyPath: \.isVPNEnabled) {}
        }
      }
    }
  }

  var isSmartProxyRoutingEnabled: Bool {
    get { BraveVPN.isSmartProxyRoutingEnabled }
    set {
      BraveVPN.isSmartProxyRoutingEnabled = newValue
      updateSelectedServer()
    }
  }

  var isKillSwitchEnabled: Bool {
    get { BraveVPN.isKillSwitchEnabled }
    set { BraveVPN.isKillSwitchEnabled = newValue }
  }

  var selectedTransportProtocol: String {
    return GRDTransportProtocol.prettyTransportProtocolString(
      for: GRDTransportProtocol.getUserPreferredTransportProtocol()
    )
  }

  private(set) var isConfigurationResetting: Bool = false

  private(set) var selectedServer: ServerDetails?

  struct ServerDetails {
    var city: String
    var country: String
    var flagISOCode: String?
    var isSmartProxyRoutingAvailable: Bool
  }

  struct SubscriptionDetails: Equatable {
    var status: String
    var statusColor: UIColor
    var expirationDate: String
  }

  var subscriptionDetails: SubscriptionDetails?

  var isAppStoreReceiptAvailable: Bool {
    store.vpnSubscriptionStatus?.state != nil
  }

  var isDevReceiptLinkingAvailable: Bool {
    store.environment != .production
  }

  init(
    store: BraveStoreSDK,
    skusService: (any SkusSkusService)? = Skus.SkusServiceFactory.get(privateMode: false),
    calendar: Calendar = .autoupdatingCurrent
  ) {
    self.store = store
    self.skusService = skusService
    self.calendar = calendar

    vpnStatusDidChangeObservable = NotificationCenter.default.addObserver(
      forName: .NEVPNStatusDidChange,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      self?.vpnStatusDidChange()
    }

    updateSelectedServer()

    Task {
      await updateSubscriptionDetails()
    }
  }

  func updateSelectedServer() {
    let hostname = BraveVPN.hostname?.components(separatedBy: ".").first ?? "-"
    let locationCity = BraveVPN.serverLocationDetailed.city ?? "-"
    let locationCountry = BraveVPN.serverLocationDetailed.country ?? hostname
    var isSmartProxyRoutingAvailable = false
    if let activatedRegion = BraveVPN.activatedRegion {
      isSmartProxyRoutingAvailable =
        !activatedRegion.smartRoutingProxyState.isEmpty
        && activatedRegion.smartRoutingProxyState != kGRDRegionSmartRoutingProxyNone
    }
    selectedServer = .init(
      city: locationCity,
      country: locationCountry,
      flagISOCode: BraveVPN.serverLocation.isoCode,
      isSmartProxyRoutingAvailable: isSmartProxyRoutingAvailable
    )
  }

  @MainActor
  func updateSubscriptionDetails() async {
    var credentialSummary: SkusCredentialSummary?

    if store.vpnSubscriptionStatus == nil, let skusService = skusService {
      do {
        credentialSummary = try await skusService.credentialsSummary(for: .vpn)
      } catch {
        Logger.module.error("Error Fetching VPN Skus Credential Summary: \(error)")
      }
    }

    let (status, statusColor) = getProductStatusInfo()
    let expiration = getExpirationDate(credentialSummary: credentialSummary)

    subscriptionDetails = SubscriptionDetails(
      status: status,
      statusColor: statusColor,
      expirationDate: expiration
    )
  }

  @MainActor
  func resetConfiguraton() async -> Bool {
    isConfigurationResetting = true
    defer { isConfigurationResetting = false }
    // This allows the UI to update immediately since BraveVPN.reconfigureVPN will block the main
    // thread temporarily
    try? await Task.sleep(for: .milliseconds(100))
    let success = await withCheckedContinuation { continuation in
      BraveVPN.reconfigureVPN {
        continuation.resume(returning: $0)
      }
    }
    updateSelectedServer()
    return success
  }

  private func getProductStatusInfo() -> (status: String, color: UIColor) {
    if Preferences.VPN.vpnReceiptStatus.value
      == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue
    {
      return (Strings.VPN.vpnActionUpdatePaymentMethodSettingsText, .braveErrorLabel)
    }

    if BraveVPN.vpnState == .expired {
      return (Strings.VPN.subscriptionStatusExpired, .braveErrorLabel)
    } else {
      return (BraveVPN.subscriptionName, .braveLabel)
    }
  }

  private func getExpirationDate(credentialSummary: SkusCredentialSummary?) -> String {
    let dateFormatter = DateFormatter().then {
      $0.locale = Locale.current
      $0.dateStyle = .short
    }

    if let order = credentialSummary?.order, let expiresAt = order.expiresAt {
      Preferences.VPN.expirationDate.value = expiresAt
      return dateFormatter.string(from: expiresAt)
    }

    guard let expirationDate = Preferences.VPN.expirationDate.value else {
      return BraveVPN.vpnState == .expired ? "-" : ""
    }

    return dateFormatter.string(from: expirationDate)
  }

  private func vpnStatusDidChange() {
    withMutation(keyPath: \.isVPNEnabled) {}
    withMutation(keyPath: \.selectedTransportProtocol) {}
  }
}

struct SmartProxyBadge: View {
  var body: some View {
    Image(braveSystemName: "leo.smart.proxy-routing")
      .imageScale(.small)
      .foregroundColor(Color(braveSystemName: .iconDefault))
      .padding(4)
      .background(
        Color(braveSystemName: .containerHighlight),
        in: .rect(cornerRadius: 4, style: .continuous)
      )
  }
}

public struct VPNSettingsView: View {
  @State private var isResetDialogPresented: Bool = false
  @State private var resetConfigurationResult: ResetConfigurationResult?
  @Bindable var viewModel: VPNSettingsViewModel

  var openURL: (URL) -> Void

  fileprivate enum ResetConfigurationResult {
    case success
    case failure
  }

  public var body: some View {
    Form {
      Section {
        Toggle(Strings.VPN.settingsVPNEnabled, isOn: $viewModel.isVPNEnabled)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $viewModel.isSmartProxyRoutingEnabled) {
          VStack(alignment: .leading) {
            HStack {
              Text(Strings.VPN.settingsVPNSmartProxyEnabled)
              SmartProxyBadge()
            }
            // Contains markdown
            Text(
              LocalizedStringKey(
                String.localizedStringWithFormat(
                  Strings.VPN.settingsVPNSmartProxyDescription,
                  URL.brave.braveVPNSmartProxySupport.absoluteString
                )
              )
            )
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $viewModel.isKillSwitchEnabled) {
          VStack(alignment: .leading) {
            Text(Strings.VPN.settingsVPNKillSwitchTitle)
            // Contains markdown
            Text(
              LocalizedStringKey(
                String.localizedStringWithFormat(
                  Strings.VPN.settingsVPNKillSwitchDescription,
                  URL.brave.braveVPNKillSwitchSupport.absoluteString
                )
              )
            )
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
      }
      .disabled(viewModel.isConfigurationResetting)
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      SubscriptionSection(viewModel: viewModel)
      Section {
        NavigationLink {
          BraveVPNRegionListView { _ in
            viewModel.updateSelectedServer()
          }
        } label: {
          Label {
            VStack(alignment: .leading) {
              HStack {
                Text(viewModel.selectedServer?.country ?? "-")
                if viewModel.selectedServer?.isSmartProxyRoutingAvailable == true {
                  SmartProxyBadge()
                }
              }
              Text(viewModel.selectedServer?.city ?? "-")
                .font(.footnote)
                .foregroundStyle(Color(braveSystemName: .textSecondary))
            }
            .frame(maxWidth: .infinity, alignment: .leading)
          } icon: {
            if let regionFlag = viewModel.selectedServer?.flagISOCode?.regionFlagImage {
              Image(uiImage: regionFlag)
            } else {
              Image(braveSystemName: "leo.globe")
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        NavigationLink {
          UIKitController(BraveVPNProtocolPickerViewController())
        } label: {
          LabeledContent(Strings.VPN.settingsTransportProtocol) {
            Text(viewModel.selectedTransportProtocol)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Button {
          isResetDialogPresented = true
        } label: {
          HStack {
            Text(Strings.VPN.settingsResetConfiguration)
              .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
            Spacer()
            if viewModel.isConfigurationResetting {
              ProgressView()
                .progressViewStyle(.circular)
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .confirmationDialog(
          Strings.VPN.vpnResetAlertTitle,
          isPresented: $isResetDialogPresented,
          titleVisibility: .visible
        ) {
          Button(Strings.VPN.vpnResetButton, role: .destructive) {
            tappedResetConfiguration()
          }
          Button(Strings.CancelString, role: .cancel) {}
        } message: {
          Text(Strings.VPN.vpnResetAlertBody)
        }
      } header: {
        Text(Strings.VPN.settingsServerSection)
      }
      .disabled(viewModel.isConfigurationResetting)
      Section {
        NavigationLink(Strings.VPN.settingsContactSupport) {
          VPNContactFormView()
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Button(Strings.VPN.settingsFAQ) {
          openURL(.brave.braveVPNFaq)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .foregroundStyle(Color(braveSystemName: .textInteractive))
      } header: {
        Text(Strings.support)
      }
    }
    .tint(Color(braveSystemName: .primary40))
    .navigationTitle(Strings.VPN.vpnName)
    .navigationBarTitleDisplayMode(.inline)
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .alert(
      resetConfigurationResult == .success
        ? Strings.VPN.resetVPNSuccessTitle : Strings.VPN.resetVPNErrorTitle,
      isPresented: $resetConfigurationResult.isPresented,
      presenting: resetConfigurationResult
    ) { result in
      switch result {
      case .success:
        Button(Strings.OKString) {}
      case .failure:
        Button(Strings.CancelString, role: .cancel) {}
        Button(Strings.VPN.resetVPNErrorButtonActionTitle, role: .destructive) {
          tappedResetConfiguration()
        }
      }
    } message: { result in
      switch result {
      case .success:
        Text(Strings.VPN.resetVPNSuccessBody)
      case .failure:
        Text(Strings.VPN.resetVPNErrorBody)
      }
    }
    .environment(
      \.openURL,
      OpenURLAction { url in
        openURL(url)
        return .handled
      }
    )
  }

  private func tappedResetConfiguration() {
    Task {
      resetConfigurationResult = await viewModel.resetConfiguraton() ? .success : .failure
    }
  }

  private struct SubscriptionSection: View {
    @Bindable var viewModel: VPNSettingsViewModel

    @Environment(\.openURL) private var openURL

    var body: some View {
      Section {
        if let details = viewModel.subscriptionDetails {
          LabeledContent(Strings.VPN.settingsSubscriptionStatus) {
            Text(details.status)
              .foregroundStyle(Color(details.statusColor))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          LabeledContent(
            Strings.VPN.settingsSubscriptionExpiration,
            value: details.expirationDate
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          Button {
            if let url = URL.apple.manageSubscriptions, UIApplication.shared.canOpenURL(url) {
              UIApplication.shared.open(url, options: [:])
            }
          } label: {
            Text(Strings.VPN.settingsManageSubscription)
          }
          .foregroundStyle(Color(braveSystemName: .textInteractive))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          Button {
            SKPaymentQueue.default().presentCodeRedemptionSheet()
          } label: {
            Text(Strings.VPN.settingsRedeemOfferCode)
          }
          .foregroundStyle(Color(braveSystemName: .textInteractive))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          Button {
            Task {
              try await BraveVPNInAppPurchaseObserver.refreshReceipt()
            }
            openURL(.brave.braveVPNLinkReceiptProd)
          } label: {
            Text(Strings.VPN.settingsLinkReceipt)
          }
          .foregroundStyle(Color(braveSystemName: .textInteractive))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          if viewModel.isDevReceiptLinkingAvailable {
            Button {
              Task {
                try await BraveVPNInAppPurchaseObserver.refreshReceipt()
              }
              openURL(.brave.braveVPNLinkReceiptStaging)
            } label: {
              Text("[Staging] Link Receipt")
            }
            .foregroundStyle(Color(braveSystemName: .textInteractive))
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
            Button {
              Task {
                try await BraveVPNInAppPurchaseObserver.refreshReceipt()
              }
              openURL(.brave.braveVPNLinkReceiptDev)
            } label: {
              Text("[Dev] Link Receipt")
            }
            .foregroundStyle(Color(braveSystemName: .textInteractive))
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }

          if viewModel.isAppStoreReceiptAvailable {
            NavigationLink {
              StoreKitReceiptSimpleView()
            } label: {
              Text(Strings.VPN.settingsViewReceipt)
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        } else {
          ProgressView()
            .progressViewStyle(.circular)
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.VPN.settingsSubscriptionSection)
      } footer: {
        Text(Strings.VPN.settingsLinkReceiptFooter)
      }
    }
  }
}

// Binding<ResetConfigurationResult?> presentation helper
extension VPNSettingsView.ResetConfigurationResult? {
  fileprivate var isPresented: Bool {
    get { self != nil }
    set { if !newValue { self = nil } }
  }
}

public class BraveVPNSettingsViewController: UIHostingController<VPNSettingsView> {
  public var openURL: ((URL) -> Void)?

  public init(skusService: SkusSkusService?, openURL: @escaping (URL) -> Void) {
    super.init(
      rootView: .init(
        viewModel: .init(store: .init(skusService: skusService), skusService: skusService),
        openURL: openURL
      )
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override func viewDidLoad() {
    super.viewDidLoad()
    title = Strings.VPN.vpnName
  }
}

#if DEBUG
#Preview {
  NavigationStack {
    VPNSettingsView(
      viewModel: .init(
        store: .init(skusService: nil),
        skusService: nil
      ),
      openURL: { _ in }
    )
  }
}
#endif
