// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveStrings
import BraveUI
import DesignSystem

struct PlaylistFolderSharingManagementView: View {
  private struct UX {
    static let hPadding = 20.0
    static let vPadding = 20.0
    static let vMidSpacing = 6.0
  }
  
  @State private var contentSize: CGSize = .zero
  
  var onAddToPlaylistPressed: (() -> Void)?
  var onSettingsPressed: (() -> Void)?
  var onCancelPressed: (() -> Void)?
  
  var body: some View {
    ScrollView(.vertical) {
      VStack {
        Text(Strings.PlaylistFolderSharing.offlineManagementViewTitle)
          .font(.title2.weight(.medium))
          .multilineTextAlignment(.center)
          .foregroundColor(Color(.bravePrimary))
          .padding(.horizontal, UX.hPadding)
          .padding(.bottom, UX.vPadding)
        
        Text(Strings.PlaylistFolderSharing.offlineManagementViewDescription)
          .font(.body)
          .foregroundColor(Color(.braveLabel))
          .padding(.horizontal, UX.hPadding)
          .padding(.bottom, UX.vPadding)
        
        VStack(spacing: UX.vMidSpacing) {
          Button(action: {
            onAddToPlaylistPressed?()
          }) {
            Text(Strings.PlaylistFolderSharing.offlineManagementViewAddButtonTitle)
              .foregroundColor(Color(.bravePrimary))
              .frame(maxWidth: .infinity)
          }
          .frame(minHeight: 44.0)
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
          
          Button(action: {
            onSettingsPressed?()
          }) {
            Text(Strings.PlaylistFolderSharing.offlineManagementViewSettingsButtonTitle)
              .foregroundColor(Color(.bravePrimary))
              .frame(maxWidth: .infinity)
          }
          .frame(minHeight: 44.0)
          .buttonStyle(BraveOutlineButtonStyle(size: .normal))
          
          Button(action: {
            onCancelPressed?()
          }) {
            Text(Strings.cancelButtonTitle)
              .frame(maxWidth: .infinity)
              .font(BraveButtonSize.normal.font)
              .foregroundColor(Color(.bravePrimary))
              .padding(BraveButtonSize.normal.padding)
              .clipShape(Capsule())
              .contentShape(Capsule())
          }
          .frame(minHeight: 44.0)
        }
        .padding(.horizontal, UX.hPadding)
      }
      .accentColor(Color(.white))
      .padding(EdgeInsets(top: UX.vPadding, leading: UX.hPadding, bottom: UX.vPadding, trailing: UX.hPadding))
      .background(
        GeometryReader { geometry in
          Color(.braveBackground)
            .onAppear {
              contentSize = geometry.size
            }
            .onChange(of: geometry.size) { size in
              contentSize = size
            }
        }
      )
    }
    .frame(maxHeight: contentSize.height)
    .environment(\.colorScheme, .dark)
  }
}

#if DEBUG
struct PlaylistFolderSharingManagementView_Previews: PreviewProvider {
  static var previews: some View {
    PlaylistFolderSharingManagementView(onAddToPlaylistPressed: nil,
                                        onSettingsPressed: nil,
                                        onCancelPressed: nil)
  }
}
#endif
