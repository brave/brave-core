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

  @State
  var sheetHeight: CGFloat = 0.0

  var body: some View {
    ScrollView {
      VStack(spacing: 16.0) {
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
                  .padding(.vertical, 12.0)

                Spacer()

                Image(braveSystemName: "leo.carat.right")
                  .foregroundStyle(Color(braveSystemName: .iconDefault))
              }
              .padding(.horizontal, 16.0)
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
      }
      .padding(24.0)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .onGeometryChange(
        for: CGSize.self,
        of: { $0.size },
        action: {
          sheetHeight = $0.height
        }
      )
    }
    .presentationDetents([.height(sheetHeight)])
    .presentationDragIndicator(.visible)
    .osAvailabilityModifiers({
      if #available(iOS 16.4, *) {
        $0.presentationBackground(.thickMaterial)
          .presentationCornerRadius(15.0)
          .presentationCompactAdaptation(.sheet)
      } else {
        $0.background(.thickMaterial)
      }
    })
  }
}
