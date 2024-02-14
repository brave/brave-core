// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import BraveShared
import DesignSystem
import BraveUI

public struct VPNLinkReceiptView: View {
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  public var linkReceiptAction: (() -> Void)?
  
  public init() {}
  
  public var body: some View {
    ScrollView {
      VStack(spacing: 24) {
        Image("link_receipt_image", bundle: .module)
        Text(Strings.Onboarding.linkReceiptTitle)
          .font(.title.weight(.medium))
          .multilineTextAlignment(.center)
        Text(Strings.Onboarding.linkReceiptDescription)
          .font(.subheadline)
          .multilineTextAlignment(.center)
        
        Button(action: {
          linkReceiptAction?()
          presentationMode.dismiss()
        }) {
          Text(Strings.Onboarding.linkReceiptButton)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
           
        HStack(spacing: 8) {
          Text(Strings.VPN.poweredBy)
            .font(.footnote)
            .foregroundColor(Color(.bravePrimary))
            .multilineTextAlignment(.center)
          Image(sharedName: "vpn_brand")
            .renderingMode(.template)
            .foregroundColor(Color(.bravePrimary))
        }
      }
      .padding(32)
    }
    .background(Color(.braveBackground))
    .frame(height: 650)
    .overlay {
      Button {
        presentationMode.dismiss()
      } label: {
        Image(braveSystemName: "leo.close")
          .renderingMode(.template)
          .foregroundColor(Color(.bravePrimary))
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
      .padding([.top, .trailing], 20)
    }
  }
}

#if DEBUG
struct VPNLinkReceiptView_Previews: PreviewProvider {
  static var previews: some View {
    VPNLinkReceiptView()
      .previewLayout(.sizeThatFits)
  }
}
#endif
