// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import DeviceCheck
import SwiftUI

struct RewardsInternalsView: View {
  let rewardsAPI: BraveRewardsAPI

  @State private var internalsInfo: BraveCore.BraveRewards.RewardsInternalsInfo?
  @State private var isSharePresented: Bool = false

  var body: some View {
    Form {
      if let info = internalsInfo {
        Section {
          VStack(alignment: .leading) {
            Text(Strings.RewardsInternals.sharingWarningTitle)
              .font(.headline)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
            Text(Strings.RewardsInternals.sharingWarningMessage)
              .font(.subheadline)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section {
          LabeledContent(
            Strings.RewardsInternals.keyInfoSeed,
            value:
              "\(info.isKeyInfoSeedValid ? Strings.RewardsInternals.valid : Strings.RewardsInternals.invalid)"
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          LabeledContent(Strings.RewardsInternals.walletPaymentID) {
            Text(info.paymentId)
              .textSelection(.enabled)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          LabeledContent(
            Strings.RewardsInternals.walletCreationDate,
            value: Date(timeIntervalSince1970: TimeInterval(info.bootStamp)),
            format: .dateTime.year().month().day()
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.RewardsInternals.walletInfoHeader)
        }
        Section {
          LabeledContent(
            Strings.RewardsInternals.status,
            value: DCDevice.current.isSupported
              ? Strings.RewardsInternals.supported : Strings.RewardsInternals.notSupported
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          LabeledContent(
            Strings.RewardsInternals.enrollmentState,
            value: DeviceCheckClient.isDeviceEnrolled()
              ? Strings.RewardsInternals.enrolled : Strings.RewardsInternals.notEnrolled
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.RewardsInternals.deviceInfoHeader)
        }
      } else {
        ProgressView()
      }
    }
    .task {
      internalsInfo = await rewardsAPI.rewardsInternalInfo()
    }
    .toolbar {
      ToolbarItemGroup(placement: .topBarTrailing) {
        Button {
          isSharePresented = true
        } label: {
          Label(Strings.RewardsInternals.shareInternalsTitle, braveSystemImage: "leo.share.macos")
        }
      }
    }
    .navigationTitle(Strings.RewardsInternals.title)
    .sheet(isPresented: $isSharePresented) {
      RewardsInternalsShareRepresentable(rewardsAPI: rewardsAPI)
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
  }
}

private struct RewardsInternalsShareRepresentable: UIViewControllerRepresentable {
  var rewardsAPI: BraveRewardsAPI

  func makeUIViewController(context: Context) -> some UIViewController {
    let controller = RewardsInternalsShareController(
      rewardsAPI: rewardsAPI,
      initiallySelectedSharables: RewardsInternalsSharable.default
    )
    return UINavigationController(rootViewController: controller)
  }

  func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
  }
}

/// A file generator that creates a JSON file containing basic information such as wallet info, device info
/// and balance info
struct RewardsInternalsBasicInfoGenerator: RewardsInternalsFileGenerator {
  func generateFiles(
    at path: String,
    using builder: RewardsInternalsSharableBuilder
  ) async throws {
    // Only 1 file to make here
    let info = try await withCheckedThrowingContinuation { continuation in
      builder.rewardsAPI.rewardsInternalInfo { internals in
        if let internals {
          continuation.resume(returning: internals)
        } else {
          continuation.resume(throwing: RewardsInternalsSharableError.rewardsInternalsUnavailable)
        }
      }
    }

    let data: [String: Any] = [
      "Rewards Profile Info": [
        "Key Info Seed": "\(info.isKeyInfoSeedValid ? "Valid" : "Invalid")",
        "Rewards Payment ID": info.paymentId,
        "Rewards Profile Creation Date": builder.dateFormatter.string(
          from: Date(timeIntervalSince1970: TimeInterval(info.bootStamp))
        ),
      ],
      "Device Info": [
        "DeviceCheck Status": DCDevice.current.isSupported ? "Supported" : "Not supported",
        "DeviceCheck Enrollment State": DeviceCheckClient.isDeviceEnrolled()
          ? "Enrolled" : "Not enrolled",
        "OS": "\(await UIDevice.current.systemName) \(await UIDevice.current.systemVersion)",
        "Model": await UIDevice.current.model,
      ],
    ]

    try await builder.writeJSON(from: data, named: "basic", at: path)
  }
}
