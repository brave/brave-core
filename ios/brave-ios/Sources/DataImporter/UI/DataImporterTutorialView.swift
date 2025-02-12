// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

private struct DataImporterTutorialStepView: View {
  var icon: Image
  var stepNumber: Int
  var stepDescription: String

  var body: some View {
    VStack {
      HStack {
        icon

        Text(String(format: Strings.DataImporter.importTutorialStepTitle, stepNumber))
          .font(.caption2.weight(.bold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }
      .padding(8.0)
      .background(Color(braveSystemName: .pageBackground), in: .capsule)

      Text(LocalizedStringKey(stepDescription))
        .font(.subheadline)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .fixedSize(horizontal: false, vertical: true)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(
      Color(braveSystemName: .containerBackground),
      in: RoundedRectangle(cornerRadius: 16.0, style: .continuous)
    )
  }
}

struct DataImporterTutorialView: View {

  @Environment(\.openURL)
  private var openURL

  var body: some View {
    ScrollView {
      VStack(spacing: 12.0) {
        DataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.grid05"),
          stepNumber: 1,
          stepDescription: Strings.DataImporter.importTutorialStepOneTitle
        )

        DataImporterTutorialStepView(
          icon: Image("safari_icon", bundle: .module),
          stepNumber: 2,
          stepDescription: Strings.DataImporter.importTutorialStepTwoTitle
        )

        DataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.arrow.diagonal-up-right"),
          stepNumber: 3,
          stepDescription: Strings.DataImporter.importTutorialStepThreeTitle
        )

        DataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.check.circle-outline"),
          stepNumber: 4,
          stepDescription:
            Strings.DataImporter.importTutorialStepFourTitle
        )

        DataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.folder"),
          stepNumber: 5,
          stepDescription:
            Strings.DataImporter.importTutorialStepFiveTitle
        )
        .padding(.bottom, 16.0)

        Text(Strings.DataImporter.importTutorialDetailedProcessTitle)
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
          .fixedSize(horizontal: false, vertical: true)
          .frame(maxWidth: .infinity, maxHeight: .infinity)

        Button(
          action: {
            openURL(URL.Apple.dataImportSupport)
          },
          label: {
            Text(Strings.DataImporter.importTutorialDetailedProcessMessage)
              .font(.subheadline.weight(.semibold))
              .multilineTextAlignment(.center)
              .foregroundStyle(Color(braveSystemName: .textInteractive))
              .fixedSize(horizontal: false, vertical: true)
              .frame(maxWidth: .infinity, maxHeight: .infinity)
          }
        )
      }
      .padding(12.0)
      .navigationTitle(Strings.DataImporter.importTutorialScreenTitle)
    }
    .background(Color(braveSystemName: .pageBackground))
  }
}

#if DEBUG
#Preview {
  DataImporterTutorialView()
}
#endif
