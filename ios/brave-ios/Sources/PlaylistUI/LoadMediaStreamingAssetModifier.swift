// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Playlist
import SwiftUI

/// Possible errors that can be thrown when calling ``EnvironmentValues/loadMediaStreamingAsset``
enum LoadMediaStreamingAssetActionError: Error {
  /// The action was called without first preparing the media streamer using the
  /// `prepareMediaStreamer` view modifier higher in the View hierarchy
  case mediaStreamerNotPrepared
}

/// An action that requests an up to date streaming URL from a given playlist item
///
/// Use the ``EnvironmentValues/loadMediaStreamingAsset`` environment value to get an instance
/// of this action in the current Environment. Call the instance directly to perform the request.
///
/// For example:
///
///     @Environment(\.loadMediaStreamingAsset) private var loadMediaStreamingAsset
///     var item: PlaylistInfo
///
///     var body: some View {
///       Button {
///         Task {
///           let updatedItem = try await loadMediaStreamingAsset(item: item)
///           play(updatedItem.src)
///         }
///       } label: {
///         Text("Play")
///       }
///     }
struct LoadMediaStreamingAssetAction {
  // FIXME: WeakBox?
  fileprivate var mediaStreamer: PlaylistMediaStreamer?

  /// Attempts to fetch an up to date streaming URL for the given `PlaylistInfo` if no cache exists
  /// for said item.
  ///
  /// Do not call this method, instead use the Swift language feature to call it directly from
  /// the instance. E.g. `loadMediaStreamingAsset(item: item)`
  func callAsFunction(item: PlaylistInfo) async throws -> PlaylistInfo {
    guard let mediaStreamer else {
      throw LoadMediaStreamingAssetActionError.mediaStreamerNotPrepared
    }
    return try await mediaStreamer.loadMediaStreamingAsset(item)
  }
}

extension EnvironmentValues {
  private struct LoadMediaStreamingAssetActionKey: EnvironmentKey {
    static var defaultValue: LoadMediaStreamingAssetAction = .init()
  }

  /// The action to allow you to load a streaming asset for a given playlist item
  ///
  /// - Note: This environment value will not work unless a parent View uses
  ///         the `prepareMediaStreamer` modifier.
  var loadMediaStreamingAsset: LoadMediaStreamingAssetAction {
    self[LoadMediaStreamingAssetActionKey.self]
  }

  /// Writable reference to `loadMediaStreamingAsset`
  fileprivate var _loadMediaStreamingAsset: LoadMediaStreamingAssetAction {
    get { self[LoadMediaStreamingAssetActionKey.self] }
    set { self[LoadMediaStreamingAssetActionKey.self] = newValue }
  }
}

extension View {
  /// Allows the view access to the `loadMediaStreamingAsset` environement value
  func prepareMediaStreamer(webLoaderFactory: any PlaylistWebLoaderFactory) -> some View {
    modifier(LoadMediaStreamingAssetModifier(webLoaderFactory: webLoaderFactory))
  }
}

private struct LoadMediaStreamingAssetModifier: ViewModifier {
  var webLoaderFactory: any PlaylistWebLoaderFactory
  @State private var mediaStreamer: PlaylistMediaStreamer?

  func body(content: Content) -> some View {
    content
      .environment(\._loadMediaStreamingAsset, .init(mediaStreamer: mediaStreamer))
      .background {
        _RepresentableView(webLoaderFactory: webLoaderFactory, mediaStreamer: $mediaStreamer)
          // Can't use the `hidden` modifier or the view isn't added at all
          .opacity(0)
          .accessibilityHidden(true)
      }
  }

  private struct _RepresentableView: UIViewRepresentable {
    var webLoaderFactory: any PlaylistWebLoaderFactory
    @Binding var mediaStreamer: PlaylistMediaStreamer?

    func makeUIView(context: Context) -> UIView {
      .init()
    }

    func updateUIView(_ uiView: UIView, context: Context) {
      // Avoid infinite update loop by ensuring we haven't already set this
      if mediaStreamer == nil {
        // Dispatch off main to avoid updating state during body computation
        DispatchQueue.main.async {
          mediaStreamer = .init(playerView: uiView, webLoaderFactory: webLoaderFactory)
        }
      }
    }
  }
}
