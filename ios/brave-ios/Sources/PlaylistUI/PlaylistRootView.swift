// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Playlist
import SwiftUI

public class PlaylistHostingController: UIHostingController<PlaylistRootView> {
  public init(
    player: PlayerModel,
    delegate: PlaylistRootView.Delegate
  ) {
    super.init(
      rootView: PlaylistRootView(
        player: player,
        delegate: delegate
      )
    )
    // FIXME: This needs to be over fullscreen because the webview to load streaming urls is added to BVC, which needs to be in the view hierarchy
    // Maybe alter the web loader/streamer setup to ask for a UIView to put the web view on instead?
    modalPresentationStyle = .overFullScreen
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    // SwiftUI will handle the background
    view.backgroundColor = .clear
  }

  deinit {
    // FIXME: Remove in the future
    print("PlaylistHostingController deinit")
  }
}

public struct PlaylistRootView: View {
  /// Methods for handling playlist related actions that the browser should handle
  public struct Delegate {
    /// Open a URL in a tab (optionally in private mode)
    public var openTabURL: (URL, _ isPrivate: Bool) -> Void
    /// Called when playlist is dismissed
    public var onDismissal: () -> Void

    public init(
      openTabURL: @escaping (URL, _ isPrivate: Bool) -> Void,
      onDismissal: @escaping () -> Void
    ) {
      self.openTabURL = openTabURL
      self.onDismissal = onDismissal
    }
  }

  var player: PlayerModel
  private var delegate: Delegate

  public init(
    player: PlayerModel,
    delegate: Delegate
  ) {
    self.player = player
    self.delegate = delegate
  }

  public var body: some View {
    PlaylistContentView(
      playerModel: player
    )
    .preparePlaylistEnvironment()
    .environment(\.managedObjectContext, DataController.swiftUIContext)
    .environment(\.colorScheme, .dark)
    .preferredColorScheme(.dark)
    .environment(
      \.openTabURL,
      .init { [delegate] url, isPrivate in
        delegate.openTabURL(url, isPrivate)
      }
    )
    .onDisappear {
      delegate.onDismissal()
    }
    .onAppear {
      if player.isPictureInPictureActive {
        player.stopPictureInPicture()
      }
    }
    .onReceive(
      NotificationCenter.default.publisher(
        for: UIApplication.didBecomeActiveNotification
      ),
      perform: { _ in
        if player.isPictureInPictureActive {
          player.stopPictureInPicture()
        }
      }
    )
  }
}

extension View {
  /// Adds playlist-specific environment variables
  ///
  /// Use this if you need access to playlist environment variables such as `isFullScreen`,
  /// `requestGeometryUpdate` or `interfaceOrientation` in a SwiftUI Preview
  func preparePlaylistEnvironment() -> some View {
    self
      .prepareEffectiveGeometryEnvironment()
      .prepareFullScreenEnvironment()
  }
}

#if DEBUG
// swift-format-ignore
#Preview {
  PlaylistRootView(
    player: .preview,
    delegate: .init(
      openTabURL: { _, _ in },
      onDismissal: { }
    )
  )
}
#endif
