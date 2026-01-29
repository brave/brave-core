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
import Web

struct SubmitReportView: View {
  @Environment(\.dismiss) private var dismiss: DismissAction
  let url: URL
  let isPrivateBrowsing: Bool
  private weak var tab: (any TabState)?

  init(url: URL, isPrivateBrowsing: Bool, tab: (any TabState)?) {
    self.url = url
    self.isPrivateBrowsing = isPrivateBrowsing
    self.tab = tab
  }

  @ScaledMetric private var textEditorHeight = 80.0
  @State private var additionalDetails = ""
  @State private var contactDetails = ""
  @State private var isSubmittingReport = false
  @State private var isSubmitted = false
  @State private var isContactInfoDescVisible: Bool = false
  @State private var categories: [WebcompatReporter.WebcompatCategoryItem] = []
  @State private var selectedCategory: WebcompatReporter.WebcompatCategoryItem?

  private var isSubmitDisabled: Bool {
    return isSubmittingReport || selectedCategory == nil
      || (selectedCategory?.additionalDetailsRequired == true && additionalDetails.isEmpty)
  }

  var body: some View {
    NavigationStack {
      ScrollView {
        VStack(alignment: .leading, spacing: 16) {
          Text(Strings.Shields.reportBrokenSiteBody1)
          Text(url.absoluteString)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
          Text(Strings.Shields.reportBrokenSiteBody2)
            .font(.footnote)
          VStack(alignment: .leading) {
            Text(Strings.Shields.reportBrokenCategory)
              .font(.caption)
            Menu(
              content: {
                Picker(
                  "",
                  selection: $selectedCategory,
                  content: {
                    ForEach(
                      categories,
                      id: \.self
                    ) { category in
                      Text(category.localizedTitle)
                        .tag(category)
                    }
                  }
                )
              },
              label: {
                HStack {
                  if let selectedCategory {
                    Text(selectedCategory.localizedTitle)
                      .foregroundStyle(Color(braveSystemName: .textPrimary))
                  } else {
                    Text(Strings.Shields.reportBrokenPlaceholder)
                      .foregroundColor(Color(.placeholderText))
                  }
                  Spacer()
                  Image(systemName: "chevron.up.chevron.down")
                    .foregroundColor(Color(.secondaryBraveLabel))
                }
              }
            )
            .braveInputStyle()
          }
          BraveTextEditor(
            text: $additionalDetails,
            prompt: selectedCategory?.additionalDetailsRequired == true
              ? Strings.Shields.reportBrokenAdditionalDetailsRequired
              : Strings.Shields.reportBrokenAdditionalDetails
          ).frame(height: textEditorHeight)

          VStack(alignment: .leading, spacing: 4) {
            Text(Strings.Shields.reportBrokenContactMe).font(.caption)
            TextField(
              Strings.Shields.reportBrokenContactMe,
              text: $contactDetails,
              prompt: Text(Strings.Shields.reportBrokenContactMeSuggestions)
            )
            .textContentType(.emailAddress)
            .textFieldStyle(
              BraveTextFieldStyle(
                strokeColor: .clear,
                backgroundColor: Color(braveSystemName: .containerBackground),
                cornerRadius: 8
              )
            )
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
                self.categories = await webcompatReporterAPI.webcompatCategories()
                let browserParams = await webcompatReporterAPI.browserParams()
                self.contactDetails = browserParams.0 ?? ""
                self.isContactInfoDescVisible = browserParams.1
              }
            }
            if self.isContactInfoDescVisible {
              Text(Strings.Shields.reportBrokenContactMeDescription).font(.caption)
            }
          }
        }
        .padding()
      }
      .navigationDestination(
        isPresented: $isSubmitted,
        destination: {
          SubmitReportSuccessView()
            .navigationBarHidden(true)
        }
      )
      .background(Color(braveSystemName: .iosBrowserChromeBackgroundIos))
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
              Task { @MainActor in
                await createAndSubmitReport()
              }
            }
          ).disabled(isSubmitDisabled)
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
      .navigationBarTitleDisplayMode(.inline)
    }
  }

  @MainActor func createAndSubmitReport() async {
    guard !isSubmitDisabled,
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
    let isBlockScriptsEnabled =
      (tab?.braveShieldsHelper?.isShieldExpected(
        for: tab?.visibleURL,
        shield: .noScript,
        considerAllShieldsOption: false
      ) ?? false)
    let isBlockFingerprintingEnabled =
      (tab?.braveShieldsHelper?.isShieldExpected(
        for: tab?.visibleURL,
        shield: .fpProtection,
        considerAllShieldsOption: false
      ) ?? true)
    let adblkList = FilterListStorage.shared.filterLists
      .compactMap({ return $0.isEnabled ? $0.entry.title : nil })
      .joined(separator: ",")
    isSubmittingReport = true
    webcompatReporterAPI.submitWebcompatReport(
      reportInfo: .init(
        channel: AppConstants.buildChannel.webCompatReportName,
        braveVersion: version,
        reportUrl: url.absoluteString,
        shieldsEnabled: String(
          tab?.braveShieldsHelper?.isBraveShieldsEnabled(for: tab?.visibleURL) ?? true
        ),
        adblockOnlyModeEnabled: nil,
        adBlockSetting: (tab?.braveShieldsHelper?.shieldLevel(
          for: tab?.visibleURL,
          considerAllShieldsOption: false
        ) ?? .standard).reportLabel,
        fpBlockSetting: isBlockFingerprintingEnabled
          ? ShieldLevel.standard.reportLabel : ShieldLevel.disabled.reportLabel,
        adBlockListNames: adblkList,
        languages: Locale.current.language.languageCode?.identifier,
        languageFarbling: String(true),
        braveVpnConnected: String(BraveVPN.isConnected),
        category: selectedCategory?.value,
        details: additionalDetails,
        contact: contactDetails,
        cookiePolicy: Preferences.Privacy.blockAllCookies.value ? "block" : nil,
        blockScripts: String(isBlockScriptsEnabled),
        adBlockComponentsVersion: nil,
        screenshotPng: nil,
        webcompatReporterErrors: nil
      )
    )
    isSubmittingReport = false
    isSubmitted = true
    // auto-dismiss report modal after 4-seconds
    try? await Task.sleep(seconds: 4)
    dismiss()
  }
}

#Preview {
  SubmitReportView(
    url: URL(string: "https://brave.com/privacy-features")!,
    isPrivateBrowsing: false,
    tab: TabStateFactory.create(with: .init())
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

extension WebcompatReporter.WebcompatCategoryItem {
  fileprivate var additionalDetailsRequired: Bool {
    category == .other
  }
}
