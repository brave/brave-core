// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import BraveVPN
import Data
import DesignSystem
import Preferences
import Shared
import Strings
import SwiftUI

struct SubmitReportView: View {
  @Environment(\.dismiss) private var dismiss: DismissAction
  let url: URL
  let isPrivateBrowsing: Bool

  @ScaledMetric private var textEditorHeight = 80.0
  @State private var additionalDetails = ""
  @State private var contactDetails = ""
  @State private var isSubmittingReport = false
  @State private var isSubmitted = false
  @State private var isContactInfoDescVisible: Bool = false

  private var scrollContent: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 16) {
        Text(Strings.Shields.reportBrokenSiteBody1)
        Text(url.absoluteString)
          .foregroundStyle(Color(braveSystemName: .textInteractive))
        Text(Strings.Shields.reportBrokenSiteBody2)
          .font(.footnote)
        BraveTextEditor(
          text: $additionalDetails,
          prompt: Strings.Shields.reportBrokenAdditionalDetails
        ).frame(height: textEditorHeight)

        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.Shields.reportBrokenContactMe).font(.caption)
          TextField(
            Strings.Shields.reportBrokenContactMe,
            text: $contactDetails,
            prompt: Text(Strings.Shields.reportBrokenContactMeSuggestions)
          )
          .textContentType(.emailAddress)
          .textFieldStyle(BraveTextFieldStyle())
          .autocorrectionDisabled()
          .textInputAutocapitalization(.never)
          .onAppear {
            guard
              let webcompatReporterAPI = WebcompatReporter.ServiceFactory.get(
                privateMode: isPrivateBrowsing
              )
            else {
              return
            }
            Task { @MainActor in
              let contactInfo = await webcompatReporterAPI.contactInfo()
              self.contactDetails = contactInfo.0 ?? ""
              self.isContactInfoDescVisible = contactInfo.1
            }
          }
          if self.isContactInfoDescVisible {
            Text(Strings.Shields.reportBrokenContactMeDescription).font(.caption)
          }
        }
      }
      .padding()
    }
    .background {
      // When we drop iOS 16, we can use NavigationStack with .navigationDestination
      // For now we use a hack available to iOS 16
      NavigationLink(isActive: $isSubmitted) {
        SubmitReportSuccessView()
          .navigationBarHidden(true)
      } label: {
        EmptyView()
      }
    }
    .background(Color(.braveBackground))
    .foregroundStyle(Color(braveSystemName: .textSecondary))
    .navigationTitle(Strings.Shields.reportABrokenSite)
    .toolbar {
      ToolbarItem(placement: .cancellationAction) {
        Button(Strings.cancelButtonTitle) {
          dismiss()
        }
        .disabled(isSubmittingReport)
      }

      ToolbarItem(placement: .confirmationAction) {
        Button(
          Strings.Shields.reportBrokenSubmitButtonTitle,
          action: {
            isSubmittingReport = true

            Task { @MainActor in
              await createAndSubmitReport()

              isSubmitted = true
              try await Task.sleep(seconds: 4)
              dismiss()
            }
          }
        ).disabled(isSubmittingReport)
      }
    }
    .overlay {
      if isSubmittingReport {
        ProgressView()
          .progressViewStyle(.braveCircular(size: .normal, tint: .braveBlurpleTint))
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .background(Color(.braveBackground).opacity(0.5).ignoresSafeArea())
          .transition(.opacity.animation(.default))
      }
    }
  }

  var body: some View {
    NavigationView {
      scrollContent.navigationBarTitleDisplayMode(.inline)
    }
    .navigationViewStyle(.stack)
  }

  @MainActor func createAndSubmitReport() async {
    let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
    guard
      let webcompatReporterAPI = WebcompatReporter.ServiceFactory.get(
        privateMode: isPrivateBrowsing
      )
    else {
      return
    }
    let version = String(
      format: "%@ (%@)",
      AppInfo.appVersion,
      AppInfo.buildNumber
    )
    let adblkList = FilterListStorage.shared.filterLists
      .compactMap({ return $0.isEnabled ? $0.entry.title : nil })
      .joined(separator: ",")
    webcompatReporterAPI.submitWebcompatReport(
      reportInfo: .init(
        channel: AppConstants.buildChannel.webCompatReportName,
        braveVersion: version,
        reportUrl: url.absoluteString,
        shieldsEnabled: String(!domain.areAllShieldsOff),
        adBlockSetting: domain.globalBlockAdsAndTrackingLevel.reportLabel,
        fpBlockSetting: domain.finterprintProtectionLevel.reportLabel,
        adBlockListNames: adblkList,
        languages: Locale.current.language.languageCode?.identifier,
        languageFarbling: String(true),
        braveVpnConnected: String(BraveVPN.isConnected),
        details: additionalDetails,
        contact: contactDetails,
        cookiePolicy: Preferences.Privacy.blockAllCookies.value ? "block" : nil,
        blockScripts: String(Preferences.Shields.blockScripts.value),
        adBlockComponentsVersion: nil,
        screenshotPng: nil
      )
    )
  }
}

#Preview {
  SubmitReportView(
    url: URL(string: "https://brave.com/privacy-features")!,
    isPrivateBrowsing: false
  )
}

extension ShieldLevel {
  /// The value that is sent to the webcompat report server
  fileprivate var reportLabel: String {
    switch self {
    case .aggressive: return "aggressive"
    case .standard: return "standard"
    case .disabled: return "allow"
    }
  }
}

extension AppBuildChannel {
  fileprivate var webCompatReportName: String {
    return self == .debug ? "developer" : rawValue
  }
}
