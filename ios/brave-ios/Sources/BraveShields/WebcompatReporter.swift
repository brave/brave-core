// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared
import os.log

public class WebcompatReporterUploader {
  static let log = Logger(
    subsystem: Bundle.main.bundleIdentifier!,
    category: "WebcompatReporterUploader"
  )

  /// Report a webcompat issue on a given website
  public static func send(
    braveVersion: String?,
    reportUrl: String?,
    fpBlockSetting: ShieldLevel?,
    shieldsEnabled: Bool?,
    adBlockSetting: ShieldLevel?,
    adBlockListNames: String?,
    languages: String?,
    languageFarbling: Bool?,
    braveVpnConnected: Bool?,
    additionalDetails: String?,
    contactInfo: String?,
    adBlockComponentsVersion: [WebcompatReporter.ComponentInfo]?,
    screenshotPng: [NSNumber]?
  ) async {

    let webcompatReporterAPI = await WebcompatReporter.ServiceFactory.get(
      privateMode: false
    )!
    webcompatReporterAPI.submitWebcompatReport(
      reportInfo: .init(
        channel: AppConstants.buildChannel.webCompatReportName,
        braveVersion: braveVersion,
        reportUrl: reportUrl,
        shieldsEnabled: shieldsEnabled != nil
          ? String(
            shieldsEnabled!
          ) : "",
        adBlockSetting: adBlockSetting != nil ? adBlockSetting!.reportLabel : "",
        fpBlockSetting: fpBlockSetting != nil ? fpBlockSetting!.reportLabel : "",
        adBlockListNames: adBlockListNames,
        languages: Locale.current.language.languageCode?.identifier,
        languageFarbling: String(
          languageFarbling!
        ),
        braveVpnConnected: braveVpnConnected != nil
          ? String(
            braveVpnConnected!
          ) : "",
        details: additionalDetails,
        contact: contactInfo,
        adBlockComponentsVersion: nil,
        screenshotPng: screenshotPng
      )
    )
  }
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
