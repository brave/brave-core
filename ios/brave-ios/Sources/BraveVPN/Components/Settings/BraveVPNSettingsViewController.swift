// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import GuardianConnect
import Preferences
import Shared
import Static
import UIKit
import os.log
import SwiftUI

public class BraveVPNSettingsViewController: TableViewController {

  public var openURL: ((URL) -> Void)?
  
  // MARK: Internal

  private let iapObserver: BraveVPNInAppPurchaseObserver

  private var hostname: String {
    BraveVPN.hostname?.components(separatedBy: ".").first ?? "-"
  }

  private var expirationDate: String {
    guard let expirationDate = Preferences.VPN.expirationDate.value else {
      return ""
    }

    let dateFormatter = DateFormatter().then {
      $0.locale = Locale.current
      $0.dateStyle = .short
    }

    return dateFormatter.string(from: expirationDate)
  }
  
  private var vpnReconfigurationPending = false {
    didSet {
      DispatchQueue.main.async {
        self.vpnConnectionStatusToggle?.isEnabled = !self.vpnReconfigurationPending
      }
    }
  }
  
  private var vpnConnectionStatusToggle: SwitchAccessoryView?
  
  private var vpnKillSwitchStatusToggle: SwitchAccessoryView?

  /// Loading view with an transparent overlay
  private var overlayView: UIView?

  private var isLoading: Bool = false {
    didSet {
      overlayView?.removeFromSuperview()
      overlayView = nil
      tableView.isScrollEnabled = true

      if !isLoading { return }
      
      let loaderView = LoaderView(size: .normal).then {
        $0.tintColor = UIColor(braveSystemName: .iconInteractive)
      }

      let overlay = UIView().then {
        $0.backgroundColor = UIColor.white.withAlphaComponent(0.4)
      }

      view.addSubview(overlay)
      overlay.addSubview(loaderView)

      overlay.frame = CGRect(size: tableView.contentSize)
      loaderView.frame.origin = 
        CGPoint(x: tableView.bounds.midX - (loaderView.bounds.width / 2), y: tableView.bounds.midY)
      
      loaderView.start()
      overlayView = overlay
      tableView.isScrollEnabled = false
    }
  }
  
  // MARK: Section Details
  
  private let vpnStatusSectionCellId = "status"
  private let serverSectionId = "server"
  private let hostCellId = "host"
  private let locationCellId = "location"
  private let protocolCellId = "protocol"
  private let resetCellId = "reset"
  private let killSwitchSectionCellId = "killswitch"
  
  // Section - VPN Status
  
  private var vpnStatusSection: Static.Section {
    let statusSwitchView = SwitchAccessoryView(
      initialValue: BraveVPN.isConnected,
      valueChange: { vpnOn in
        if vpnOn {
          BraveVPN.reconnect()
        } else {
          BraveVPN.disconnect()
        }
      }
    )

    if Preferences.VPN.vpnReceiptStatus.value
      == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue
    {
      statusSwitchView.onTintColor = .braveErrorLabel
    }

    vpnConnectionStatusToggle = statusSwitchView

    return Section(
      rows: [
        Row(
          text: Strings.VPN.settingsVPNEnabled,
          accessory: .view(statusSwitchView),
          uuid: vpnStatusSectionCellId
        )
      ],
      uuid: vpnStatusSectionCellId
    )
  }
  
  // Section - Subscription
  
  private var subscriptionSection: Static.Section {
    let (subscriptionStatus, statusDetailColor) = { () -> (String, UIColor) in
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
    }()

    let expiration = BraveVPN.vpnState == .expired ? "-" : expirationDate
    
    var linkReceiptRows: [Row] {
      var rows = [Row]()

      rows.append(
        Row(
          text: Strings.VPN.settingsLinkReceipt,
          selection: { [unowned self] in
            openURL?(.brave.braveVPNLinkReceiptProd)
          },
          cellClass: ButtonCell.self
        )
      )

      if BraveVPN.isSandbox {
        rows += [
          Row(
            text: "[Staging] Link Receipt",
            selection: { [unowned self] in
              openURL?(.brave.braveVPNLinkReceiptStaging)
            },
            cellClass: ButtonCell.self
          ),
          Row(
            text: "[Dev] Link Receipt",
            selection: { [unowned self] in
              openURL?(.brave.braveVPNLinkReceiptDev)
            },
            cellClass: ButtonCell.self
          ),
        ]
      }

      return rows
    }

    return Section(
      header: .title(Strings.VPN.settingsSubscriptionSection),
      rows: [
        Row(
          text: Strings.VPN.settingsSubscriptionStatus,
          detailText: subscriptionStatus,
          cellClass: ColoredDetailCell.self,
          context: [ColoredDetailCell.colorKey: statusDetailColor]
        ),
        Row(text: Strings.VPN.settingsSubscriptionExpiration, detailText: expiration),
        Row(
          text: Strings.VPN.settingsManageSubscription,
          selection: {
            guard let url = URL.apple.manageSubscriptions else { return }
            if UIApplication.shared.canOpenURL(url) {
              // Opens Apple's 'manage subscription' screen.
              UIApplication.shared.open(url, options: [:])
            }
          },
          cellClass: ButtonCell.self
        ),
        Row(
          text: Strings.VPN.settingsRedeemOfferCode,
          selection: {
            self.isLoading = false
            // Open the redeem code sheet
            SKPaymentQueue.default().presentCodeRedemptionSheet()
          },
          cellClass: ButtonCell.self
        ),
      ] + linkReceiptRows,
      footer: .title(Strings.VPN.settingsLinkReceiptFooter)
    )
  }
  
  // Section - Server
  
  private var serverSection: Static.Section {
    let locationCity = BraveVPN.serverLocationDetailed.city ?? "-"
    let locationCountry = BraveVPN.serverLocationDetailed.country ?? hostname

    let userPreferredTunnelProtocol = GRDTransportProtocol.getUserPreferredTransportProtocol()
    let transportProtocol = GRDTransportProtocol.prettyTransportProtocolString(
      for: userPreferredTunnelProtocol
    )

    return Section(
      header: .title(Strings.VPN.settingsServerSection),
      rows: [
        Row(
          text: locationCountry,
          detailText: locationCity,
          selection: { [unowned self] in
            self.selectServerTapped()
          },
          image: BraveVPN.serverLocation.isoCode?.regionFlagImage
            ?? UIImage(braveSystemNamed: "leo.globe"),
          accessory: .disclosureIndicator,
          cellClass: MultilineSubtitleCell.self,
          uuid: locationCellId
        ),
        Row(
          text: Strings.VPN.settingsTransportProtocol,
          detailText: transportProtocol,
          selection: { [unowned self] in
            self.selectProtocolTapped()
          },
          accessory: .disclosureIndicator,
          uuid: protocolCellId
        ),
        Row(
          text: Strings.VPN.settingsResetConfiguration,
          selection: { [unowned self] in
            self.resetConfigurationTapped()
          },
          cellClass: ButtonCell.self,
          uuid: resetCellId
        ),
      ],
      uuid: serverSectionId
    )
  }
  
  private var killSwitchSection: Static.Section {
    let killSwitchView = SwitchAccessoryView(
      initialValue: true,
      valueChange: { [weak self] killSwitchON in
        guard let self = self else { return }
                
        BraveVPN.helper.killSwitchEnabled = killSwitchON
        
        self.performAllNetworkReconnect()
      }
    )
    
    vpnKillSwitchStatusToggle = killSwitchView
    
    return Section(
      header: .title(Strings.VPN.settingsKillSwitchTitle.capitalized),
      rows: [
        Row(
          text: Strings.VPN.settingsKillSwitchTitle,
          accessory: .view(killSwitchView),
          uuid: killSwitchSectionCellId
        )
      ],
      footer: .title(Strings.VPN.settingsKillSwitchDescription),
      uuid: killSwitchSectionCellId
    )
  }
  
  private var techSupportSection: Static.Section {
    return Section(
      header: .title(Strings.VPN.settingsSupportSection),
      rows: [
        Row(
          text: Strings.VPN.settingsContactSupport,
          selection: { [unowned self] in
            self.sendContactSupportEmail()
          },
          accessory: .disclosureIndicator
        ),
        Row(
          text: Strings.VPN.settingsFAQ,
          selection: { [unowned self] in
            self.openURL?(.brave.braveVPNFaq)
          },
          cellClass: ButtonCell.self
        )
      ])
  }
  
  // MARK: Lifecycle
  
  public init(iapObserver: BraveVPNInAppPurchaseObserver) {
    self.iapObserver = iapObserver
    super.init(style: .insetGrouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.VPN.vpnName
    
    // Add VPN Status Observer
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(vpnConfigChanged(_:)),
      name: .NEVPNStatusDidChange,
      object: nil
    )

    // Insert Settings Sections
    dataSource.sections = [
      vpnStatusSection,
      subscriptionSection,
      serverSection,
      killSwitchSection,
      techSupportSection
    ]
  }
  
  /// Reconnecting VPN after Kill Switch toggle is set
  private func performAllNetworkReconnect() {
   isLoading = true

    BraveVPN.reconnect() { [weak self] connected in
      guard let self = self else { return }
        
      DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
        self.isLoading = false
        
        guard connected else {
          self.showEnableKillSwitchError()
          return
        }
        
        // Normally the connected server should not change
        // This is added in case there is an unexpected change happens on server side
        self.updateServerSectionInfo()
      }
    }
  }

  private func updateServerSectionInfo() {
    guard
      let hostIndexPath =
        dataSource
        .indexPath(rowUUID: hostCellId, sectionUUID: serverSectionId)
    else { return }

    guard
      let locationIndexPath =
        dataSource
        .indexPath(rowUUID: locationCellId, sectionUUID: serverSectionId)
    else { return }

    dataSource.sections[hostIndexPath.section].rows[hostIndexPath.row]
      .detailText = hostname
    dataSource.sections[locationIndexPath.section].rows[locationIndexPath.row]
      .detailText = BraveVPN.serverLocation.hostName ?? "-"
  }
}

// MARK: - Actions

extension BraveVPNSettingsViewController {
  private func sendContactSupportEmail() {
    navigationController?.pushViewController(BraveVPNContactFormViewController(), animated: true)
  }

  private func resetConfigurationTapped() {
    let alert = UIAlertController(
      title: Strings.VPN.vpnResetAlertTitle,
      message: Strings.VPN.vpnResetAlertBody,
      preferredStyle: .actionSheet
    )

    let cancel = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel)
    let reset = UIAlertAction(
      title: Strings.VPN.vpnResetButton,
      style: .destructive,
      handler: { [weak self] _ in
        guard let self = self else { return }
        self.vpnReconfigurationPending = true
        self.isLoading = true
        Logger.module.debug("Reconfiguring the vpn")

        BraveVPN.reconfigureVPN { success in
          Logger.module.debug("Reconfiguration succeeded: \(success)")
          // Small delay before unlocking UI because enabling vpn
          // takes a moment after we call to connect to the vpn.
          DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
            self.isLoading = false
            self.vpnReconfigurationPending = false
            self.updateServerSectionInfo()
            self.showVPNResetAlert(success: success)
          }
        }
      }
    )

    alert.addAction(cancel)
    alert.addAction(reset)

    if UIDevice.current.userInterfaceIdiom == .pad {
      alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]

      if let resetCellIndexPath =
        dataSource
        .indexPath(rowUUID: resetCellId, sectionUUID: serverSectionId)
      {
        let cell = tableView.cellForRow(at: resetCellIndexPath)

        alert.popoverPresentationController?.sourceView = cell
        alert.popoverPresentationController?.sourceRect = cell?.bounds ?? .zero
      } else {
        alert.popoverPresentationController?.sourceView = self.view
        alert.popoverPresentationController?.sourceRect = self.view.bounds
      }
    }

    present(alert, animated: true)
  }

  private func selectServerTapped() {
    if BraveVPN.allRegions.isEmpty {
      let alert = UIAlertController(
        title: Strings.VPN.vpnConfigGenericErrorTitle,
        message: Strings.VPN.settingsFailedToFetchServerList,
        preferredStyle: .alert
      )
      alert.addAction(.init(title: Strings.OKString, style: .default))
      // Failed to fetch regions at app init or first vpn configuration.
      // Let's try again here.
      BraveVPN.populateRegionDataIfNecessary()
      present(alert, animated: true)
      return
    }

    let vc = UIHostingController(rootView: BraveVPNRegionListView())
    vc.title = Strings.VPN.vpnRegionListServerScreenTitle
    navigationController?.pushViewController(vc, animated: true)
  }

  private func selectProtocolTapped() {
    let vc = BraveVPNProtocolPickerViewController()
    navigationController?.pushViewController(vc, animated: true)
  }

  private func showVPNResetAlert(success: Bool) {
    let alert = UIAlertController(
      title: success ? Strings.VPN.resetVPNSuccessTitle : Strings.VPN.resetVPNErrorTitle,
      message: success ? Strings.VPN.resetVPNSuccessBody : Strings.VPN.resetVPNErrorBody,
      preferredStyle: .alert
    )

    if success {
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default))
    } else {
      alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))
      alert.addAction(
        UIAlertAction(
          title: Strings.VPN.resetVPNErrorButtonActionTitle,
          style: .default,
          handler: { [weak self] _ in
            self?.resetConfigurationTapped()
          }
        )
      )
    }

    present(alert, animated: true)
  }
  
  private func showEnableKillSwitchError() {
    let alert = UIAlertController(
      title: Strings.VPN.settingsKillSwitchToggleErrorTitle,
      message: Strings.VPN.settingsKillSwitchToggleErrorDescription,
      preferredStyle: .alert
    )

    let cancel = UIAlertAction(
      title: Strings.cancelButtonTitle,
      style: .cancel,
      handler: { [weak self] _ in
        guard let self = self, let toggleStatus = self.vpnKillSwitchStatusToggle?.isOn else {
          return
        }
        
        // Re-try canceled
        // Kill switch and UI is set  previous toggle status
        BraveVPN.helper.killSwitchEnabled = !toggleStatus
        self.vpnKillSwitchStatusToggle?.isOn = !toggleStatus
    })
    
    let retry = UIAlertAction(
      title: Strings.VPN.settingsRetryActionTitle,
      style: .default,
      handler: { [weak self] _ in
        self?.performAllNetworkReconnect()
      }
    )

    alert.addAction(cancel)
    alert.addAction(retry)

    present(alert, animated: true)
  }
}

// MARK: - VPNConnection Observer

extension BraveVPNSettingsViewController {
  @objc func vpnConfigChanged(_ notification: NSNotification) {
    guard let vpnConnection = notification.object as? NEVPNConnection else {
      return
    }

    switch vpnConnection.status {
    case .connecting, .disconnecting, .reasserting:
      isLoading = true
    case .invalid:
      vpnConnectionStatusToggle?.isOn = false
      isLoading = false
    case .connected, .disconnected:
      vpnConnectionStatusToggle?.isOn = BraveVPN.isConnected
      isLoading = false
    @unknown default:
      assertionFailure()
      break
    }
  }
}

// MARK: - IAPObserverDelegate

extension BraveVPNSettingsViewController: BraveVPNInAppPurchaseObserverDelegate {
  public func purchasedOrRestoredProduct(validateReceipt: Bool) {
    DispatchQueue.main.async {
      self.isLoading = false
    }

    if validateReceipt {
      BraveVPN.validateReceiptData()
    }
  }

  public func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError) {
    // Handle Offer Code error
    guard isLoading else {
      return
    }

    handleOfferCodeError(error: error)
  }

  public func handlePromotedInAppPurchase() {
    // No-op
  }

  private func handleOfferCodeError(error: BraveVPNInAppPurchaseObserver.PurchaseError) {
    DispatchQueue.main.async {
      self.isLoading = false

      let message = Strings.VPN.vpnErrorOfferCodeFailedBody

      let alert = UIAlertController(
        title: Strings.VPN.vpnErrorPurchaseFailedTitle,
        message: message,
        preferredStyle: .alert
      )
      let ok = UIAlertAction(title: Strings.OKString, style: .default, handler: nil)
      alert.addAction(ok)
      self.present(alert, animated: true)
    }
  }
}
