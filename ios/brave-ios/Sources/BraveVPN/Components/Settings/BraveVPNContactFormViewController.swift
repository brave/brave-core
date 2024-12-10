// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
// To get cellular carrier name
import CoreTelephony
import Foundation
import GuardianConnect
import MessageUI
import Shared
import Static
import UIKit
import os.log

class BraveVPNContactFormViewController: TableViewController {

  private let supportEmail = "braveios@guardianapp.com"

  init() {
    super.init(style: .grouped)
  }

  private struct ContactForm {
    var hostname: String?
    var tunnelProtocol: String?
    var subscriptionType: String?
    var receipt: String?
    var appVersion: String?
    var timezone: String?
    var networkType: String?
    var cellularCarrier: String?
    var issue: String?
    var logs: [(date: Date, message: String)]?
  }

  /// User can choose an issue from predefined list or write their own in email body.
  private enum IssueType: String, RepresentableOptionType, CaseIterable {
    case otherConnectionProblems
    case noInternet
    case slowConnection
    case websiteProblems
    case other

    var displayString: String {
      switch self {
      case .otherConnectionProblems: return Strings.VPN.contactFormIssueOtherConnectionError
      case .noInternet: return Strings.VPN.contactFormIssueNoInternet
      case .slowConnection: return Strings.VPN.contactFormIssueSlowConnection
      case .websiteProblems: return Strings.VPN.contactFormIssueWebsiteProblems
      case .other: return Strings.VPN.contactFormIssueOther
      }
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) { fatalError() }

  private var contactForm = ContactForm()

  override func viewDidLoad() {
    // MARK: Hostname
    let hostname = BraveVPN.hostname
    let hostnameRow = Row(
      text: Strings.VPN.contactFormHostname,
      accessory:
        .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.hostname = hostname
              } else {
                self?.contactForm.hostname = nil
              }
            }
          )
        )
    )

    // MARK: TunnelProtocol
    let userPreferredTunnelProtocol = GRDTransportProtocol.getUserPreferredTransportProtocol()
    let transportProtocol = GRDTransportProtocol.prettyTransportProtocolString(
      for: userPreferredTunnelProtocol
    )
    let tunnelProtocolRow =
      Row(
        text: Strings.VPN.protocolPickerTitle,
        detailText: transportProtocol,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.tunnelProtocol = transportProtocol
              } else {
                self?.contactForm.tunnelProtocol = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    // MARK: SubscriptionType
    let subscriptionType = BraveVPN.subscriptionName
    let subscriptionTypeRow =
      Row(
        text: Strings.VPN.contactFormSubscriptionType,
        detailText: subscriptionType,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.subscriptionType = subscriptionType
              } else {
                self?.contactForm.subscriptionType = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    // MARK: App Store receipt
    let receiptRow = Row(
      text: Strings.VPN.contactFormAppStoreReceipt,
      accessory:
        .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              guard let self else { return }
              if isOn {
                Task {
                  self.contactForm.receipt = await self.fetchReceipt()
                }
              } else {
                contactForm.receipt = nil
              }
            }
          )
        )
    )

    // MARK: App Version
    let appVersion = AppInfo.appVersion
    let appVersionRow =
      Row(
        text: Strings.VPN.contactFormAppVersion,
        detailText: appVersion,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.appVersion = appVersion
              } else {
                self?.contactForm.appVersion = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    // MARK: Timezone
    let timezone = TimeZone.current.description
    let timezoneRow =
      Row(
        text: Strings.VPN.contactFormTimezone,
        detailText: timezone,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.timezone = timezone
              } else {
                self?.contactForm.timezone = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    // MARK: Network Type
    let networkType = getNetworkType
    let networkTypeRow =
      Row(
        text: Strings.VPN.contactFormNetworkType,
        detailText: networkType,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.networkType = networkType
              } else {
                self?.contactForm.networkType = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    // MARK: Cellular Carrier
    let carrierName =
      CTTelephonyNetworkInfo().serviceSubscriberCellularProviders?
      .first?.value.carrierName ?? "-"
    let carrierRow =
      Row(
        text: Strings.VPN.contactFormCarrier,
        detailText: carrierName,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.cellularCarrier = carrierName
              } else {
                self?.contactForm.cellularCarrier = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    // MARK: Error logs
    let errorLogs =
      Row(
        text: Strings.VPN.contactFormLogs,
        accessory: .view(
          SwitchAccessoryView(
            initialValue: false,
            valueChange: { [weak self] isOn in
              if isOn {
                self?.contactForm.logs = BraveVPN.errorLog
              } else {
                self?.contactForm.logs = nil
              }
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self
      )

    let section = Section(rows: [
      hostnameRow, tunnelProtocolRow, subscriptionTypeRow, receiptRow,
      appVersionRow, timezoneRow, networkTypeRow, carrierRow, errorLogs,
    ])

    let sendButton = Row(
      text: Strings.VPN.contactFormSendButton,
      selection: { [weak self] in
        guard let self = self else { return }
        let optionChanged = {
          [weak self] (vc: OptionSelectionViewController<IssueType>, option: IssueType) -> Void in
          self?.contactForm.issue = option.displayString
          self?.createEmailOutline()
        }

        let optionsVC =
          OptionSelectionViewController<IssueType>(
            headerText: Strings.VPN.contactFormIssue,
            footerText: Strings.VPN.contactFormIssueDescription,
            options: IssueType.allCases,
            optionChanged: optionChanged
          )

        self.navigationController?.pushViewController(optionsVC, animated: true)

      },
      cellClass: CenteredButtonCell.self
    )

    let footerText =
      "\(Strings.VPN.contactFormFooterSharedWithGuardian)\n\n\(Strings.VPN.contactFormFooter)"
    let buttonSection = Section(rows: [sendButton], footer: .title(footerText))

    dataSource.sections = [section, buttonSection]
  }

  private func createEmailOutline() {
    if !MFMailComposeViewController.canSendMail() {
      Logger.module.error("Can't send email on this device")
      let alert = UIAlertController(
        title: Strings.genericErrorTitle,
        message: Strings.VPN.contactFormEmailNotConfiguredBody,
        preferredStyle: .alert
      )
      let okAction = UIAlertAction(title: Strings.OKString, style: .default)
      alert.addAction(okAction)
      present(alert, animated: true)
      return
    }

    let mail = MFMailComposeViewController().then {
      $0.mailComposeDelegate = self
      $0.setToRecipients([self.supportEmail])
    }

    var formTitle = Strings.VPN.contactFormTitle
    if let issue = contactForm.issue {
      formTitle += " + \(issue)"
    }

    mail.setSubject(formTitle)
    mail.setMessageBody(self.composeEmailBody(with: self.contactForm), isHTML: false)
    present(mail, animated: true)
  }

  private var getNetworkType: String {
    let status = Reach().connectionStatus()

    switch status {
    case .offline, .unknown:
      return "-"
    case .online(let type):
      return type.description
    }
  }

  nonisolated private func fetchReceipt() async -> String? {
    guard let receiptUrl = Bundle.main.appStoreReceiptURL else { return nil }
    do {
      return try Data(contentsOf: receiptUrl).base64EncodedString()
    } catch {
      Logger.module.error("\(error.localizedDescription)")
      return nil
    }
  }

  private func composeEmailBody(with contactForm: ContactForm) -> String {
    var body = "\n"

    body.append(contentsOf: "#### \(Strings.VPN.contactFormDoNotEditText) ####\n\n")

    body.append(Strings.VPN.contactFormPlatform)
    body.append("\n\(UIDevice.current.systemName)\n\n")

    if let issue = contactForm.issue {
      body.append(Strings.VPN.contactFormIssue)
      body.append("\n\(issue)\n\n")
    }

    if let hostname = contactForm.hostname {
      body.append(Strings.VPN.contactFormHostname)
      body.append("\n\(hostname)\n\n")
    }

    if let tunnelProtocol = contactForm.tunnelProtocol {
      body.append(Strings.VPN.protocolPickerTitle)
      body.append("\n\(tunnelProtocol)\n\n")
    }

    if let subcriptionType = contactForm.subscriptionType {
      body.append(Strings.VPN.contactFormSubscriptionType)
      body.append("\n\(subcriptionType)\n\n")
    }

    if let appVersion = contactForm.appVersion {
      body.append(Strings.VPN.contactFormAppVersion)
      body.append("\n\(appVersion)\n\n")
    }

    if let timezone = contactForm.timezone {
      body.append(Strings.VPN.contactFormTimezone)
      body.append("\n\(timezone)\n\n")
    }

    if let networkType = contactForm.networkType {
      body.append(Strings.VPN.contactFormNetworkType)
      body.append("\n\(networkType)\n\n")
    }

    if let carrier = contactForm.cellularCarrier {
      body.append(Strings.VPN.contactFormCarrier)
      body.append("\n\(carrier)\n\n")
    }

    if let logs = contactForm.logs {
      body.append("\(Strings.VPN.contactFormLogs)\n")
      let formatter = DateFormatter()
      formatter.dateStyle = .short
      formatter.timeStyle = .long

      logs.forEach {
        body.append("\(formatter.string(from: $0.date)): \($0.message)\n")
      }

      body.append("\n")
    }

    if let receipt = contactForm.receipt {
      body.append(Strings.VPN.contactFormAppStoreReceipt)
      body.append("\n\(receipt)\n\n")
    }

    return body
  }
}

// MARK: - MFMailComposeViewControllerDelegate
extension BraveVPNContactFormViewController: MFMailComposeViewControllerDelegate {
  func mailComposeController(
    _ controller: MFMailComposeViewController,
    didFinishWith result: MFMailComposeResult,
    error: Error?
  ) {
    controller.dismiss(animated: true)
  }
}
