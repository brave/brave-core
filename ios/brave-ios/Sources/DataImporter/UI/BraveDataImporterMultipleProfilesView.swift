// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct BraveDataImporterMultipleProfilesView: View {
  var zipFileExtractedURL: URL
  var profiles: [String: [String: URL]]
  var onProfileSelected: (String) -> Void

  @Environment(\.dismissHandler)
  private var dismissHandler

  var body: some View {
    VStack {
      HStack {
        Spacer()

        Button {
          dismissHandler?()
        } label: {
          Image(braveSystemName: "leo.close")
            .foregroundColor(Color(braveSystemName: .iconDefault))
            .padding(8)
        }
        .frame(alignment: .trailing)
        .background(Color(braveSystemName: .materialSeparator))
        .clipShape(Circle())
      }

      Image(
        "multi_profile_logo",
        bundle: .module
      )
      .padding(.bottom, 24.0)

      Text(Strings.DataImporter.multipleProfilesTitle)
        .font(.body.weight(.semibold))
        .multilineTextAlignment(.center)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)

      Text(Strings.DataImporter.multipleProfilesMessage)
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)

      VStack {
        ForEach(Array(profiles.keys.sorted().enumerated()), id: \.element) { (offset, profile) in
          Button {
            onProfileSelected(profile)
          } label: {
            HStack {
              Image(braveSystemName: "leo.user.picture")
                .foregroundStyle(Color(braveSystemName: .iconInteractive))

              Text(profile.localizedCapitalized)
                .foregroundStyle(Color(braveSystemName: .textInteractive))

              Spacer()

              Image(braveSystemName: "leo.carat.right")
            }
            .padding(.horizontal, 16.0)
            .padding(.vertical, 11.0)
          }

          if offset != profiles.count - 1 {
            Divider().padding(.leading)
          }
        }
      }
      .background(
        Color(braveSystemName: .iosBrowserElevatedIos)
          .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
      )
      .padding()

      Button {
        dismissHandler?()
      } label: {
        Text(Strings.CancelString)
          .font(.subheadline.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
      }
      .buttonStyle(PlainButtonStyle())
    }
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(
      .thickMaterial,
      in: RoundedRectangle(cornerRadius: 15.0, style: .continuous)
    )
  }
}
