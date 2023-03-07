// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import DesignSystem
import Shared
import BraveShared

enum PlaylistPopoverState {
  case addToPlaylist
  case addedToPlaylist
}

struct PlaylistPopoverView: View {
  @State var state: PlaylistPopoverState

  var onPrimaryButtonPressed: (() -> Void)?
  var onSecondaryButtonPressed: (() -> Void)?

  var body: some View {
    Group {
      if state == .addToPlaylist {
        addToPlaylistView
      } else {
        addedToPlaylistView
      }
    }
    .frame(maxWidth: BraveUX.baseDimensionValue)
  }

  private var addToPlaylistView: some View {
    VStack(alignment: .leading) {
      VStack(spacing: 20) {
        Text(Strings.PlayList.playlistPopoverAddTitle)
          .foregroundColor(Color(UIColor.primaryButtonTint))
          .font(.title2.weight(.medium))
        Text(Strings.PlayList.playlistPopoverAddBody)
          .foregroundColor(Color(UIColor.braveLabel))
          .font(.body)
      }
      Button(action: {
        onPrimaryButtonPressed?()
      }) {
        Text(Strings.PlayList.addToPlayListAlertTitle)
          .frame(maxWidth: .infinity)
          .font(.title3.weight(.medium))
          .foregroundColor(Color(UIColor.primaryButtonTint))
          .padding()
      }
      .buttonStyle(BraveOutlineButtonStyle(size: .normal))
      .foregroundColor(Color(UIColor.secondaryButtonTint))
    }
    .padding(
      EdgeInsets(
        top: 22.0,
        leading: 30.0,
        bottom: 22.0,
        trailing: 30.0)
    )
    .background(Color(.braveBackground))
  }

  private var addedToPlaylistView: some View {
    VStack(alignment: .leading) {
      VStack(spacing: 20.0) {
        Text(Strings.PlayList.playlistPopoverAddedTitle)
          .foregroundColor(Color(UIColor.primaryButtonTint))
          .font(.title2.weight(.medium))
        Button(action: {
          onPrimaryButtonPressed?()
        }) {
          Text(Strings.PlayList.playlistPopoverOpenInBravePlaylist)
            .frame(maxWidth: .infinity)
            .font(.title3.weight(.medium))
            .foregroundColor(.white)
            .padding()
        }
        .buttonStyle(BraveFilledButtonStyle(size: .small))
        .foregroundColor(Color(UIColor.braveBlurpleTint))
      }

      Button(action: {
        onSecondaryButtonPressed?()
      }) {
        Text(Strings.PlayList.playlistPopoverRemoveFromBravePlaylist)
          .frame(maxWidth: .infinity)
          .font(.title3.weight(.medium))
          .foregroundColor(Color(UIColor.primaryButtonTint))
          .padding()
      }
      .buttonStyle(BraveOutlineButtonStyle(size: .small))
      .foregroundColor(Color(UIColor.secondaryButtonTint))
    }
    .padding(
      EdgeInsets(
        top: 22.0,
        leading: 30.0,
        bottom: 22.0,
        trailing: 30.0)
    )
    .background(Color(.braveBackground))
  }
}

#if DEBUG
struct PlaylistPopoverView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      BraveUI.PopupView {
        PlaylistPopoverView(state: .addedToPlaylist)
      }
      .previewDevice("iPhone 12 Pro")
    }
  }
}
#endif

class PlaylistPopoverViewController: UIHostingController<PlaylistPopoverView>, PopoverContentComponent {

  init(state: PlaylistPopoverState) {
    super.init(rootView: PlaylistPopoverView(state: state))
  }

  @objc required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
