// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import Shared
import BraveShared

public struct PlaylistOnboardingView: View {
  public var onButtonPressed: (() -> Void)?

  public var body: some View {
    HStack(alignment: .top, spacing: 14.0) {
      Image("welcome-view-playlist-brave-logo", bundle: .module)

      VStack(alignment: .leading, spacing: 16.0) {
        VStack(alignment: .leading, spacing: 4.0) {
          Text(Strings.Callout.playlistOnboardingViewTitle)
            .font(.title3.weight(.medium))
            .foregroundColor(Color(.bravePrimary))

          Text(Strings.Callout.playlistOnboardingViewDescription)
            .multilineTextAlignment(.leading)
            .font(.body)
            .foregroundColor(Color(.bravePrimary))
        }
        .accessibilityElement(children: .combine)

        Button(action: { onButtonPressed?() }) {
          HStack {
            Image("welcome-view-playlist-play-icon", bundle: .module)
            Text(Strings.Callout.playlistOnboardingViewButtonTitle)
              .font(.title3.weight(.medium))
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
    }
    .frame(maxWidth: BraveUX.baseDimensionValue)
    .padding()
    .background(Color(.braveBackground))
  }
}

#if DEBUG
struct PlaylistOnboardingView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      ZStack {
        Rectangle()
          .foregroundColor(.black)
          .edgesIgnoringSafeArea(.all)
        PlaylistOnboardingView()
      }
    }
  }
}
#endif

public class PlaylistOnboardingViewController: UIHostingController<PlaylistOnboardingView>, PopoverContentComponent {

  public init() {
    super.init(rootView: PlaylistOnboardingView())
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
