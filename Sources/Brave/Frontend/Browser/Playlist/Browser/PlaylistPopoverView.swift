// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import DesignSystem
import Shared
import BraveShared
import BraveStrings

struct PlaylistPopoverView: View {
  enum Action {
    case openPlaylist
    case changeFolders
    case timedOut
  }
  
  @Environment(\.dynamicTypeSize) private var dynamicTypeSize
  
  var folderName: String
  var action: ((Action) -> Void)?
  
  private func containerView<Content: View>(
    @ViewBuilder _ content: () -> Content
  ) -> some View {
    HStack(spacing: dynamicTypeSize.isAccessibilitySize ? 0 : 10) {
      Image(braveSystemName: "leo.check.circle-filled")
        .foregroundStyle(Color(.braveSuccessLabel))
        .accessibilityHidden(true)
      if dynamicTypeSize.isAccessibilitySize {
        VStack(alignment: .leading, content: content)
      } else {
        HStack(content: content)
      }
    }
    .padding(dynamicTypeSize.isAccessibilitySize ? .all : .horizontal)
  }
  
  var body: some View {
    containerView {
      Text(String.localizedStringWithFormat(Strings.PlayList.addedToPlaylistMessage, folderName))
        .foregroundStyle(Color(.braveLabel))
        .font(.callout)
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(dynamicTypeSize.isAccessibilitySize ? .horizontal : .vertical)
        .allowsHitTesting(false)
        .background(
          Color.clear
            .contentShape(Rectangle())
            .onTapGesture {
              action?(.openPlaylist)
            }
        )
      Button(Strings.PlayList.playlistPopoverChangeFoldersButtonTitle) {
        action?(.changeFolders)
      }
      .padding(dynamicTypeSize.isAccessibilitySize ? .horizontal : .vertical)
      .font(.subheadline.weight(.semibold))
      .foregroundColor(Color(.braveBlurpleTint))
      .layoutPriority(1)
    }
    .background(Color(.braveBackground))
    .accessibilityAction(named: Strings.PlayList.openInPlaylistButtonTitle) {
      action?(.openPlaylist)
    }
    .accessibilityAction(named: Strings.PlayList.changeFoldersButtonTitle) {
      action?(.changeFolders)
    }
    .task {
      // task modifier will cancel when the view is removed
      do {
        let isVoiceOverRunning = UIAccessibility.isVoiceOverRunning
        try await Task.sleep(nanoseconds: NSEC_PER_SEC * (isVoiceOverRunning ? 15 : 4))
        action?(.timedOut)
      } catch { }
    }
  }
}

extension PlaylistPopoverView: PopoverContentComponent {}

#if DEBUG
struct PlaylistPopoverView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      BraveUI.PopupView {
        PlaylistPopoverView(folderName: "Play Later", action: {
          print($0)
        })
      }
    }
  }
}
#endif
