// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct FocusP3AScreenView: View {
  @State var activeIndex = 2

  var body: some View {
    VStack {
      Image("focus-product-insight", bundle: .module)
        .resizable()
        .frame(width: 185, height: 224)
        .padding(.bottom, 24)

      VStack(spacing: 8) {
        Text("Make Brave Better")
          .font(Font.largeTitle)

        Text("Let us know which features you’re enjoying the most.")
          .font(.body.weight(.medium))
          .lineLimit(2)
          .multilineTextAlignment(.center)
          .fixedSize(horizontal: false, vertical: true)
          .foregroundColor(Color(braveSystemName: .textTertiary))
      }
      .padding(.bottom, 16)

      Spacer()

      Button(
        action: {

        },
        label: {
          Text("Continue")
            .font(.body.weight(.semibold))
            .foregroundColor(Color(.white))
            .padding()
            .foregroundStyle(.white)
            .frame(maxWidth: .infinity)
            .background(Color(braveSystemName: .buttonBackground))
        }
      )
      .clipShape(RoundedRectangle(cornerRadius: 12.0))
      .overlay(RoundedRectangle(cornerRadius: 12.0).strokeBorder(Color.black.opacity(0.2)))
      .padding(.bottom, 24)

      FocusStepsPagingIndicator(totalPages: 4, activeIndex: $activeIndex)
        .padding(.bottom, 20)
    }
    .padding(.horizontal, 20)
    .background(Color(braveSystemName: .pageBackground))
  }
}

struct FocusP3AScreenView_Previews: PreviewProvider {
  static var previews: some View {
    FocusP3AScreenView()
  }
}
