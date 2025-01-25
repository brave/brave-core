//
//  SwiftUIView.swift
//  Brave
//
//  Created by Brandon T on 2025-01-21.
//

import SwiftUI

private struct BraveDataImporterTutorialStepView: View {
  var icon: Image
  var stepNumber: Int
  var stepDescription: String

  var body: some View {
    VStack(alignment: .center) {
      HStack {
        icon

        Text("STEP \(stepNumber)")
          .font(.caption2.weight(.bold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }
      .padding(8.0)
      .background(
        Color(braveSystemName: .pageBackground),
        in: Capsule()
      )

      Text(LocalizedStringKey(stepDescription))
        .font(.subheadline)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .fixedSize(horizontal: false, vertical: true)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(braveSystemName: .containerBackground))
    .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
  }
}

struct BraveDataImporterTutorialView: View {
  var body: some View {
    ScrollView {
      VStack(spacing: 12.0) {
        BraveDataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.grid05"),
          stepNumber: 1,
          stepDescription: "Open **Settings**, scroll down, and tap **Apps**."
        )

        BraveDataImporterTutorialStepView(
          icon: Image("safari-icon", bundle: .module),
          stepNumber: 2,
          stepDescription: "From the list of apps (sorted alphabetically), find and tap **Safari**."
        )

        BraveDataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.arrow.diagonal-up-right"),
          stepNumber: 3,
          stepDescription: "Scroll down and tap **Export**."
        )

        BraveDataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.check.circle-outline"),
          stepNumber: 4,
          stepDescription:
            "**Select** the data you want to export (Bookmarks, History, Credit Cards, Passwords)."
        )

        BraveDataImporterTutorialStepView(
          icon: Image(braveSystemName: "leo.folder"),
          stepNumber: 5,
          stepDescription:
            "Save the file in **Files**, choosing a location you can easily find later."
        )
        .padding(.bottom, 16.0)

        Text("Want a detailed export process?")
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
          .fixedSize(horizontal: false, vertical: true)
          .frame(maxWidth: .infinity, maxHeight: .infinity)

        Text("Open Safari help page")
          .font(.subheadline.weight(.semibold))
          .multilineTextAlignment(.center)
          .foregroundStyle(Color(braveSystemName: .textInteractive))
          .fixedSize(horizontal: false, vertical: true)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
      .padding(12.0)
      .navigationTitle("How to export from Safari")
    }
    .background(Color(braveSystemName: .pageBackground))
  }
}

#Preview {
  BraveDataImporterTutorialView()
}
