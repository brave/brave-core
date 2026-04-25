// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
// To get cellular carrier name
import CoreTelephony
import Foundation
import GuardianConnect
import MessageUI
import OSLog
import Shared
import SwiftUI

struct VPNContactFormView: View {
  @State private var includes: DataIncludes = .init()
  @State private var issueType: IssueType?
  @State private var isEmailPresented: Bool = false
  @State private var isEmailUnavailableAlertPresented: Bool = false

  private struct DataIncludes {
    var hostname: Bool = false
    var tunnelProtocol: Bool = false
    var subscriptionType: Bool = false
    var receipt: Bool = false
    var appVersion: Bool = false
    var timezone: Bool = false
    var networkType: Bool = false
    var cellularCarrier: Bool = false
    var logs: Bool = false
  }

  /// User can choose an issue from predefined list or write their own in email body.
  private enum IssueType: String, CaseIterable {
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

  var body: some View {
    Form {
      Section {
        Toggle(Strings.VPN.contactFormHostname, isOn: $includes.hostname)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $includes.tunnelProtocol) {
          LabeledContent(Strings.VPN.protocolPickerTitle, value: tunnelProtocol)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $includes.subscriptionType) {
          LabeledContent(Strings.VPN.contactFormSubscriptionType, value: BraveVPN.subscriptionName)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(Strings.VPN.contactFormAppStoreReceipt, isOn: $includes.receipt)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $includes.appVersion) {
          LabeledContent(Strings.VPN.contactFormAppVersion, value: AppInfo.appVersion)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $includes.timezone) {
          LabeledContent(Strings.VPN.contactFormTimezone, value: TimeZone.current.description)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $includes.networkType) {
          LabeledContent(Strings.VPN.contactFormNetworkType, value: networkType)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(isOn: $includes.cellularCarrier) {
          LabeledContent(
            Strings.VPN.contactFormCarrier,
            value: CTTelephonyNetworkInfo().serviceSubscriberCellularProviders?
              .first?.value.carrierName ?? "-"
          )
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Toggle(Strings.VPN.contactFormLogs, isOn: $includes.logs)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section {
        NavigationLink(Strings.VPN.contactFormSendButton) {
          IssuePicker(issueType: $issueType)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(
          "\(Strings.VPN.contactFormFooterSharedWithGuardian)\n\n\(Strings.VPN.contactFormFooter)"
        )
      }
    }
    .tint(Color(braveSystemName: .primary40))
    .onChange(of: issueType, initial: false) {
      if issueType == nil { return }
      if MFMailComposeViewController.canSendMail() {
        isEmailPresented = true
      } else {
        isEmailUnavailableAlertPresented = true
        issueType = nil
      }
    }
    .sheet(isPresented: $isEmailPresented) {
      MailComposeRepresentable(
        subject:
          "\(Strings.VPN.contactFormTitle) + \(issueType?.displayString ?? IssueType.other.displayString)",
        recipients: ["braveios@guardianapp.com"],
        messageBody: emailMessageBody
      )
    }
    .alert(Strings.genericErrorTitle, isPresented: $isEmailUnavailableAlertPresented) {
      Button(Strings.OKString) {}
    } message: {
      Text(Strings.VPN.contactFormEmailNotConfiguredBody)
    }
  }

  private var emailMessageBody: String {
    var body = "\n"

    body.append(contentsOf: "#### \(Strings.VPN.contactFormDoNotEditText) ####\n\n")

    body.append(Strings.VPN.contactFormPlatform)
    body.append("\n\(UIDevice.current.systemName)\n\n")

    if let issue = issueType?.displayString {
      body.append(Strings.VPN.contactFormIssue)
      body.append("\n\(issue)\n\n")
    }

    if includes.hostname, let hostname = BraveVPN.hostname {
      body.append(Strings.VPN.contactFormHostname)
      body.append("\n\(hostname)\n\n")
    }

    if includes.tunnelProtocol {
      body.append(Strings.VPN.protocolPickerTitle)
      body.append("\n\(tunnelProtocol)\n\n")
    }

    if includes.subscriptionType {
      body.append(Strings.VPN.contactFormSubscriptionType)
      body.append("\n\(BraveVPN.subscriptionName)\n\n")
    }

    if includes.appVersion {
      body.append(Strings.VPN.contactFormAppVersion)
      body.append("\n\(AppInfo.appVersion)\n\n")
    }

    if includes.timezone {
      body.append(Strings.VPN.contactFormTimezone)
      body.append("\n\(TimeZone.current.description)\n\n")
    }

    if includes.networkType {
      body.append(Strings.VPN.contactFormNetworkType)
      body.append("\n\(networkType)\n\n")
    }

    if includes.cellularCarrier {
      let carrierName =
        CTTelephonyNetworkInfo().serviceSubscriberCellularProviders?
        .first?.value.carrierName ?? "-"
      body.append(Strings.VPN.contactFormCarrier)
      body.append("\n\(carrierName)\n\n")
    }

    if includes.logs {
      let logs = BraveVPN.errorLog
      body.append("\(Strings.VPN.contactFormLogs)\n")
      let formatter = DateFormatter()
      formatter.dateStyle = .short
      formatter.timeStyle = .long

      for log in logs {
        body.append("\(formatter.string(from: log.date)): \(log.message)\n")
      }

      body.append("\n")
    }

    if includes.receipt,
      let receiptUrl = Bundle.main.appStoreReceiptURL,
      let receiptData = try? Data(contentsOf: receiptUrl)
    {
      body.append(Strings.VPN.contactFormAppStoreReceipt)
      body.append("\n\(receiptData.base64EncodedString())\n\n")
    }

    return body
  }

  private var tunnelProtocol: String {
    let userPreferredTunnelProtocol = GRDTransportProtocol.getUserPreferredTransportProtocol()
    return GRDTransportProtocol.prettyTransportProtocolString(for: userPreferredTunnelProtocol)
  }

  private var networkType: String {
    let type = Reachability.shared.status.connectionType
    if type == .offline {
      return "-"
    }
    return type.description
  }

  private struct IssuePicker: View {
    @Binding var issueType: IssueType?
    @Environment(\.dismiss) private var dismiss

    var body: some View {
      Form {
        Section {
          ForEach(IssueType.allCases, id: \.self) { issue in
            Button {
              issueType = issue
              dismiss()
            } label: {
              Text(issue.displayString)
                .foregroundStyle(Color(braveSystemName: .textPrimary))
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        } header: {
          Text(Strings.VPN.contactFormIssue)
        } footer: {
          Text(Strings.VPN.contactFormIssueDescription)
        }
      }
      .scrollContentBackground(.hidden)
      .background(Color(.braveGroupedBackground))
    }
  }
}

private struct MailComposeRepresentable: UIViewControllerRepresentable {
  var subject: String
  var recipients: [String]
  var messageBody: String

  func makeUIViewController(context: Context) -> MFMailComposeViewController {
    let composer = MFMailComposeViewController()
    composer.mailComposeDelegate = context.coordinator
    return composer
  }
  func updateUIViewController(_ uiViewController: MFMailComposeViewController, context: Context) {
    uiViewController.setToRecipients(recipients)
    uiViewController.setSubject(subject)
    uiViewController.setMessageBody(messageBody, isHTML: false)
  }

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  class Coordinator: NSObject, MFMailComposeViewControllerDelegate {
    func mailComposeController(
      _ controller: MFMailComposeViewController,
      didFinishWith result: MFMailComposeResult,
      error: Error?
    ) {
      controller.dismiss(animated: true)
    }
  }
}

class BraveVPNContactFormViewController: UIHostingController<VPNContactFormView> {
  init() {
    super.init(rootView: VPNContactFormView())
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

#if DEBUG
#Preview {
  NavigationStack {
    VPNContactFormView()
  }
}
#endif
