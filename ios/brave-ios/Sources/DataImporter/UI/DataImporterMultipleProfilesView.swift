// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import DesignSystem
import SwiftUI

struct DataImporterMultipleProfilesView: View {
  var zipFileExtractedURL: URL
  var profiles: [String: [String: URL]]
  var onProfileSelected: (String) -> Void

  @Environment(\.dismiss)
  private var dismiss

  var body: some View {
    VStack {
      Button {
        dismiss()
      } label: {
        Label {
          Text(Strings.close)
        } icon: {
          Image(braveSystemName: "leo.close")
            .foregroundColor(Color(braveSystemName: .iconDefault))
            .padding(8)
        }
        .labelStyle(.iconOnly)
      }
      .background(Color(braveSystemName: .materialSeparator), in: Circle())
      .frame(maxWidth: .infinity, alignment: .trailing)

      Image("multi_profile_logo", bundle: .module)
        .padding(.bottom, 24.0)

      VStack {
        Text(Strings.DataImporter.multipleProfilesTitle)
          .font(.headline)
          .foregroundColor(Color(braveSystemName: .textPrimary))

        Text(Strings.DataImporter.multipleProfilesMessage)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
      .multilineTextAlignment(.center)
      .frame(maxWidth: .infinity)
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
            .padding(.vertical, 12.0)
          }

          if offset != profiles.count - 1 {
            Divider().padding(.leading)
          }
        }
      }
      .background(
        Color(braveSystemName: .iosBrowserElevatedIos),
        in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
      )
      .padding()

      Button {
        dismiss()
      } label: {
        Text(Strings.CancelString)
          .font(.subheadline.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
      .buttonStyle(.plain)
    }
  }
}
