// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
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
        .frame(maxWidth: 450)   
    }
    
    private var addToPlaylistView: some View {
        VStack(alignment: .leading) {
            if #available(iOS 14.0, *) {
                Text(Strings.PlayList.playlistPopoverAddTitle)
                    .foregroundColor(Color(UIColor.primaryButtonTint))
                    .font(.title2.weight(.medium))
            } else {
                Text(Strings.PlayList.playlistPopoverAddTitle)
                    .foregroundColor(Color(UIColor.primaryButtonTint))
                    .font(.headline.weight(.medium))
            }
            
            Spacer(minLength: 20.0)
            
            Text(Strings.PlayList.playlistPopoverAddBody)
                .foregroundColor(Color(UIColor.braveLabel))
                .font(.body)
            
            Spacer(minLength: 20.0)

            Button(action: {
                onPrimaryButtonPressed?()
            }) {
                if #available(iOS 14.0, *) {
                    Text(Strings.PlayList.addToPlayListAlertTitle)
                        .frame(maxWidth: .infinity)
                        .font(.title3.weight(.medium))
                        .foregroundColor(Color(UIColor.primaryButtonTint))
                        .padding()
                } else {
                    Text(Strings.PlayList.addToPlayListAlertTitle)
                        .frame(maxWidth: .infinity)
                        .font(.body.weight(.medium))
                        .foregroundColor(Color(UIColor.primaryButtonTint))
                        .padding()
                }
            }
            .buttonStyle(BraveOutlineButtonStyle(size: .normal))
            .foregroundColor(Color(UIColor.secondaryButtonTint))
        }.padding(EdgeInsets(top: 22.0,
                             leading: 30.0,
                             bottom: 22.0,
                             trailing: 30.0))
    }
    
    private var addedToPlaylistView: some View {
        VStack(alignment: .leading) {
            if #available(iOS 14.0, *) {
                Text(Strings.PlayList.playlistPopoverAddedTitle)
                    .foregroundColor(Color(UIColor.primaryButtonTint))
                    .font(.title2.weight(.medium))
            } else {
                Text(Strings.PlayList.playlistPopoverAddedTitle)
                    .foregroundColor(Color(UIColor.primaryButtonTint))
                    .font(.headline.weight(.medium))
            }
            
            Spacer(minLength: 20.0)
            
            Button(action: {
                onPrimaryButtonPressed?()
            }) {
                if #available(iOS 14.0, *) {
                    Text(Strings.PlayList.playlistPopoverOpenInBravePlaylist)
                        .frame(maxWidth: .infinity)
                        .font(.title3.weight(.medium))
                        .foregroundColor(.white)
                        .padding()
                } else {
                    Text(Strings.PlayList.playlistPopoverOpenInBravePlaylist)
                        .frame(maxWidth: .infinity)
                        .font(.body.weight(.medium))
                        .foregroundColor(.white)
                        .padding()
                }
            }
            .buttonStyle(BraveFilledButtonStyle(size: .small))
            .foregroundColor(Color(UIColor.braveBlurple))
            
            Spacer(minLength: 20.0)

            Button(action: {
                onSecondaryButtonPressed?()
            }) {
                if #available(iOS 14.0, *) {
                    Text(Strings.PlayList.playlistPopoverRemoveFromBravePlaylist)
                        .frame(maxWidth: .infinity)
                        .font(.title3.weight(.medium))
                        .foregroundColor(Color(UIColor.primaryButtonTint))
                        .padding()
                } else {
                    Text(Strings.PlayList.playlistPopoverRemoveFromBravePlaylist)
                        .frame(maxWidth: .infinity)
                        .font(.body.weight(.medium))
                        .foregroundColor(Color(UIColor.primaryButtonTint))
                        .padding()
                }
            }
            .buttonStyle(BraveOutlineButtonStyle(size: .small))
            .foregroundColor(Color(UIColor.secondaryButtonTint))
        }.padding(EdgeInsets(top: 22.0,
                             leading: 30.0,
                             bottom: 22.0,
                             trailing: 30.0))
    }
}

struct PlaylistPopoverView_Previews: PreviewProvider {
    static var previews: some View {
        PlaylistPopoverView(state: .addedToPlaylist)
    }
}

class PlaylistPopoverViewController: UIHostingController<PlaylistPopoverView> & PopoverContentComponent {
    
    init(state: PlaylistPopoverState) {
        super.init(rootView: PlaylistPopoverView(state: state))
    }
    
    @objc required dynamic init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
