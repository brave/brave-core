//
//  SwiftUIView.swift
//  Brave
//
//  Created by Brandon T on 2025-01-22.
//

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

struct BraveDataImporterLoadingView: View {
  var body: some View {
    VStack(alignment: .center) {
      ProgressView()
        .progressViewStyle(
          BraveProgressStyleCircular(thickness: 5, speed: 3.0)
        )
        .backgroundStyle(Color(braveSystemName: .blurple20))
        .foregroundStyle(Color(braveSystemName: .iconInteractive))
        .frame(width: 40, height: 40)
        .padding(.vertical, 32.0)

      Text("Importing Your Data...")
        .font(.body.weight(.semibold))
        .multilineTextAlignment(.center)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)

      Text("Please wait while we securely transfer your data.\nThis might take a few moments.")
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
        .frame(maxWidth: .infinity, alignment: .center)
        .fixedSize(horizontal: false, vertical: true)
        .padding(.horizontal, 24.0)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
    .padding()
  }
}

#Preview {
  BraveDataImporterLoadingView()
}
