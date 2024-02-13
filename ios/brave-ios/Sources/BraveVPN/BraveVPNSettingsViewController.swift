// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import Preferences
import BraveUI
import os.log
import BraveShared
import GuardianConnect

public class BraveVPNSettingsViewController: TableViewController {

  public var openURL: ((URL) -> Void)?
  let iapObserver: IAPObserver

  public init(iapObserver: IAPObserver) {
    self.iapObserver = iapObserver

    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) { fatalError() }

  // Cell/section tags so we can update them dynamically.
  private let serverSectionId = "server"
  private let hostCellId = "host"
  private let locationCellId = "location"
  private let protocolCellId = "protocol"
  private let resetCellId = "reset"
  private let vpnStatusSectionCellId = "vpnStatus"

  private var vpnConnectionStatusSwitch: SwitchAccessoryView?

  private var vpnReconfigurationPending = false {
    didSet {
      DispatchQueue.main.async {
        self.vpnConnectionStatusSwitch?.isEnabled = !self.vpnReconfigurationPending
      }
    }
  }

  /// View to show when the vpn config reset is pending.
  private var overlayView: UIView?

  private var isLoading: Bool = false {
    didSet {
      overlayView?.removeFromSuperview()

      if !isLoading { return }

      let overlay = UIView().then {
        $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)
        let activityIndicator = UIActivityIndicatorView().then {
          $0.style = .large
          $0.color = .white
          $0.startAnimating()
          $0.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        }

        $0.addSubview(activityIndicator)
      }

      view.addSubview(overlay)
      overlay.frame = CGRect(size: tableView.contentSize)

      overlayView = overlay
    }
  }
  
  private var linkReceiptRows: [Row] {
    var rows = [Row]()
    
    rows.append(Row(text: Strings.VPN.settingsLinkReceipt,
                    selection: { [unowned self] in
      openURL?(.brave.braveVPNLinkReceiptProd)
    }, cellClass: ButtonCell.self))
    
    if BraveVPN.isSandbox {
      rows += [Row(text: "[Staging] Link Receipt",
                   selection: { [unowned self] in
        openURL?(.brave.braveVPNLinkReceiptStaging)
      }, cellClass: ButtonCell.self),
               Row(text: "[Dev] Link Receipt",
                   selection: { [unowned self] in
        openURL?(.brave.braveVPNLinkReceiptDev)
      }, cellClass: ButtonCell.self)]
    }
    
    return rows
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.VPN.vpnName
    NotificationCenter.default.addObserver(self, selector: #selector(vpnConfigChanged(_:)),
                                           name: .NEVPNStatusDidChange, object: nil)
    
    let switchView = SwitchAccessoryView(initialValue: BraveVPN.isConnected, valueChange: { vpnOn in
      if vpnOn {
        BraveVPN.reconnect()
      } else {
        BraveVPN.disconnect()
      }
    })
    
    if Preferences.VPN.vpnReceiptStatus.value == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue {
      switchView.onTintColor = .braveErrorLabel
    }
    
    self.vpnConnectionStatusSwitch = switchView
    
    let vpnStatusSection = Section(rows: [
      Row(text: Strings.VPN.settingsVPNEnabled,
          accessory: .view(switchView), uuid: vpnStatusSectionCellId)
    ], uuid: vpnStatusSectionCellId)
    
    let (subscriptionStatus, statusDetailColor) = { () -> (String, UIColor) in
      if Preferences.VPN.vpnReceiptStatus.value == BraveVPN.ReceiptResponse.Status.retryPeriod.rawValue {
        return (Strings.VPN.vpnActionUpdatePaymentMethodSettingsText, .braveErrorLabel)
      }
      
      if BraveVPN.vpnState == .expired {
        return (Strings.VPN.subscriptionStatusExpired, .braveErrorLabel)
      } else {
        return (BraveVPN.subscriptionName, .braveLabel)
      }
    }()
    
    let expiration = BraveVPN.vpnState == .expired ? "-" : expirationDate
    
    let subscriptionSection =
    Section(header: .title(Strings.VPN.settingsSubscriptionSection),
            rows: [Row(text: Strings.VPN.settingsSubscriptionStatus,
                       detailText: subscriptionStatus,
                       cellClass: ColoredDetailCell.self,
                       context: [ColoredDetailCell.colorKey: statusDetailColor]),
                   Row(text: Strings.VPN.settingsSubscriptionExpiration, detailText: expiration),
                   Row(text: Strings.VPN.settingsManageSubscription,
                       selection: {
                         guard let url = URL.apple.manageSubscriptions else { return }
                           if UIApplication.shared.canOpenURL(url) {
                             // Opens Apple's 'manage subscription' screen.
                             UIApplication.shared.open(url, options: [:])
                           }
                       },
                       cellClass: ButtonCell.self),
                   Row(text: Strings.VPN.settingsRedeemOfferCode,
                       selection: {
                        self.isLoading = false
                        // Open the redeem code sheet
                        SKPaymentQueue.default().presentCodeRedemptionSheet()
                       },
                       cellClass: ButtonCell.self)
            ] + linkReceiptRows,
            footer: .title(Strings.VPN.settingsLinkReceiptFooter))
    
    let location = BraveVPN.serverLocation ?? "-"
    
    let userPreferredTunnelProtocol = GRDTransportProtocol.getUserPreferredTransportProtocol()
    let transportProtocol = GRDTransportProtocol.prettyTransportProtocolString(for: userPreferredTunnelProtocol)

    let serverSection = Section(
      header: .title(Strings.VPN.settingsServerSection),
      rows: [Row(text: Strings.VPN.settingsServerHost, detailText: hostname, uuid: hostCellId),
             Row(text: Strings.VPN.settingsServerLocation,
                 detailText: location,
                 selection: { [unowned self] in
                    self.selectServerTapped()
                 },
                 accessory: .disclosureIndicator,
                 uuid: locationCellId),
             Row(text: Strings.VPN.settingsTransportProtocol,
                 detailText: transportProtocol,
                 selection: { [unowned self] in
                    self.selectProtocolTapped()
                 },
                 accessory: .disclosureIndicator,
                 uuid: protocolCellId),
             Row(text: Strings.VPN.settingsResetConfiguration,
                 selection: { [unowned self] in
                    self.resetConfigurationTapped()
                 },
                 cellClass: ButtonCell.self, uuid: resetCellId)],
      uuid: serverSectionId)
    
    let techSupportSection = Section(
      rows: [Row(text: Strings.VPN.settingsContactSupport,
                 selection: { [unowned self] in
                   self.sendContactSupportEmail()
                 },
                 accessory: .disclosureIndicator)])
    
    let termsSection = Section(
      rows: [Row(text: Strings.VPN.settingsFAQ,
                 selection: { [unowned self] in
                   self.openURL?(.brave.braveVPNFaq)
                 },
                 cellClass: ButtonCell.self)])
    
    dataSource.sections = [vpnStatusSection,
                           subscriptionSection,
                           serverSection,
                           techSupportSection,
                           termsSection]
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

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

  private func updateServerInfo() {
    guard let hostIndexPath = dataSource
      .indexPath(rowUUID: hostCellId, sectionUUID: serverSectionId) else { return }
    
    guard let locationIndexPath = dataSource
      .indexPath(rowUUID: locationCellId, sectionUUID: serverSectionId) else { return }
    
    dataSource.sections[hostIndexPath.section].rows[hostIndexPath.row]
      .detailText = hostname
    dataSource.sections[locationIndexPath.section].rows[locationIndexPath.row]
      .detailText = BraveVPN.serverLocation ?? "-"
  }

  private func sendContactSupportEmail() {
    navigationController?.pushViewController(BraveVPNContactFormViewController(), animated: true)
  }

  private func resetConfigurationTapped() {
    let alert = UIAlertController(title: Strings.VPN.vpnResetAlertTitle,
                                  message: Strings.VPN.vpnResetAlertBody,
                                  preferredStyle: .actionSheet)
    
    let cancel = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel)
    let reset = UIAlertAction(title: Strings.VPN.vpnResetButton, style: .destructive,
                              handler: { [weak self] _ in
      guard let self = self else { return }
      self.vpnReconfigurationPending = true
      self.isLoading = true
      Logger.module.debug("Reconfiguring the vpn")
      
      BraveVPN.reconfigureVPN() { success in
        Logger.module.debug("Reconfiguration suceedeed: \(success)")
        // Small delay before unlocking UI because enabling vpn
        // takes a moment after we call to connect to the vpn.
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
          self.isLoading = false
          self.vpnReconfigurationPending = false
          self.updateServerInfo()
          self.showVPNResetAlert(success: success)
        }
      }
    })

    alert.addAction(cancel)
    alert.addAction(reset)

    if UIDevice.current.userInterfaceIdiom == .pad {
      alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]

      if let resetCellIndexPath = dataSource
        .indexPath(rowUUID: resetCellId, sectionUUID: serverSectionId) {
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
    if BraveVPN.regions.isEmpty {
      let alert = UIAlertController(title: Strings.VPN.vpnConfigGenericErrorTitle,
                                    message: Strings.VPN.settingsFailedToFetchServerList,
                                    preferredStyle: .alert)
      alert.addAction(.init(title: Strings.OKString, style: .default))
      // Failed to fetch regions at app init or first vpn configuration.
      // Let's try again here.
      BraveVPN.populateRegionDataIfNecessary()
      present(alert, animated: true)
      return
    }

    let vc = BraveVPNRegionPickerViewController()
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
      preferredStyle: .alert)

    if success {
      let okAction = UIAlertAction(title: Strings.OKString, style: .default)
      alert.addAction(okAction)
    } else {
      alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))
      alert.addAction(UIAlertAction(
        title: Strings.VPN.resetVPNErrorButtonActionTitle, style: .default,
        handler: { [weak self] _ in
          self?.resetConfigurationTapped()
      }))
    }

    present(alert, animated: true)
  }

  @objc func vpnConfigChanged(_ notification: NSNotification) {
    guard let vpnConnection = notification.object as? NEVPNConnection else {
      return
    }
        
    switch vpnConnection.status {
    case.connecting, .disconnecting, .reasserting:
      isLoading = true
    case .invalid:
      vpnConnectionStatusSwitch?.isOn = false
    case .connected, .disconnected:
      vpnConnectionStatusSwitch?.isOn = BraveVPN.isConnected
    @unknown default:
      assertionFailure()
      break
    }
    
    isLoading = false
  }
}

// MARK: - IAPObserverDelegate

extension BraveVPNSettingsViewController: IAPObserverDelegate {
  public func purchasedOrRestoredProduct(validateReceipt: Bool) {
    DispatchQueue.main.async {
      self.isLoading = false
    }
    
    if validateReceipt {
      BraveVPN.validateReceiptData()
    }
  }

  public func purchaseFailed(error: IAPObserver.PurchaseError) {
    // Handle Offer Code error
    guard isLoading else {
      return
    }
    
    handleOfferCodeError(error: error)
  }
  
  public func handlePromotedInAppPurchase() {
    // No-op
  }
  
  private func handleOfferCodeError(error: IAPObserver.PurchaseError) {
    DispatchQueue.main.async {
      self.isLoading = false

      let message = Strings.VPN.vpnErrorOfferCodeFailedBody

      let alert = UIAlertController(
        title: Strings.VPN.vpnErrorPurchaseFailedTitle,
        message: message,
        preferredStyle: .alert)
      let ok = UIAlertAction(title: Strings.OKString, style: .default, handler: nil)
      alert.addAction(ok)
      self.present(alert, animated: true)
    }
  }
}
