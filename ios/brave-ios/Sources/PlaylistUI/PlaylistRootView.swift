// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Playlist
import SwiftUI

@available(iOS 16.0, *)
public class PlaylistHostingController: UIHostingController<PlaylistRootView> {
  public init(delegate: PlaylistRootView.Delegate) {
    super.init(rootView: PlaylistRootView(delegate: delegate))
    modalPresentationStyle = .fullScreen
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

@available(iOS 16.0, *)
public struct PlaylistRootView: View {
  /// Methods for handling playlist related actions that the browser should handle
  public struct Delegate {
    /// Open a URL in a tab (optionally in private mode)
    public var openTabURL: (URL, _ isPrivate: Bool) -> Void
    /// Returns the neccessary web loader for handling reloading streams
    public var webLoaderFactory: () -> PlaylistWebLoaderFactory
    /// Called when playlist is dismissed
    public var onDismissal: () -> Void

    public init(
      openTabURL: @escaping (URL, _ isPrivate: Bool) -> Void,
      webLoaderFactory: @escaping () -> PlaylistWebLoaderFactory,
      onDismissal: @escaping () -> Void
    ) {
      self.openTabURL = openTabURL
      self.webLoaderFactory = webLoaderFactory
      self.onDismissal = onDismissal
    }
  }

  private var delegate: Delegate

  public init(delegate: Delegate) {
    self.delegate = delegate
  }

  public var body: some View {
    PlaylistContentView()
      .preparePlaylistEnvironment()
      .prepareMediaStreamer(webLoaderFactory: delegate.webLoaderFactory())
      .environment(\.managedObjectContext, DataController.swiftUIContext)
      .environment(\.colorScheme, .dark)
      .preferredColorScheme(.dark)
      .environment(
        \.openTabURL,
        .init { url, isPrivate in
          delegate.openTabURL(url, isPrivate)
        }
      )
      .onDisappear {
        delegate.onDismissal()
      }
  }
}

@available(iOS 16.0, *)
extension View {
  /// Adds playlist-specific environment variables
  ///
  /// Use this if you need access to playlist environment variables such as `isFullScreen`,
  /// `requestGeometryUpdate` or `interfaceOrientation` in a SwiftUI Preview
  func preparePlaylistEnvironment() -> some View {
    self
      .observingInterfaceOrientation()
      .creatingRequestGeometryUpdateAction()
      .setUpFullScreenEnvironment()
  }
}

#if DEBUG
class PreviewWebLoaderFactory: PlaylistWebLoaderFactory {
  class PreviewWebLoader: UIView, PlaylistWebLoader {
    func load(url: URL) async -> PlaylistInfo? { return nil }
    func stop() {}
  }
  func makeWebLoader() -> any PlaylistWebLoader {
    PreviewWebLoader()
  }
}
// swift-format-ignore
@available(iOS 16.0, *)
#Preview {
  PlaylistRootView(
    delegate: .init(
      openTabURL: { _, _ in },
      webLoaderFactory: { PreviewWebLoaderFactory() },
      onDismissal: { }
    )
  )
}
#endif
