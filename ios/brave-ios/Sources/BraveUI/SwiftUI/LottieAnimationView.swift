// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Lottie
import SwiftUI

/// A view that displays a Lottie animation.
///
/// By default the animation will begin playing immediately, control this by passing in
/// the `isPlaying` argument. You can also control the loop mode via the
/// `LottieAnimationView.loopMode` modifier
///
/// Similar to SwiftUI's `Image` View, a `LottieAnimationView` will size itself based
/// on the Lottie animation it is displaying. To change this behaviour, use the
/// `LottieAnimationView.resizable()` modifier accompanied with usual SwiftUI modifiers
/// such as `aspectRatio`, for example:
///
///     LottieAnimationView(name: "animation", bundle: .module)
///       .resizable()
///       .aspectRatio(contentMode: .fit)
public struct LottieAnimationView: View {
  public var name: String
  public var bundle: Bundle
  public var isPlaying: Bool
  
  public init(name: String, bundle: Bundle, isPlaying: Bool = true) {
    self.name = name
    self.bundle = bundle
    self.isPlaying = isPlaying
  }
  
  private struct Configuration {
    var loopMode: LottieLoopMode = .playOnce
    var resizable: Bool = false
  }
  
  private var configuration: Configuration = .init()
  
  public var body: some View {
    Representable(
      name: name,
      bundle: bundle,
      isPlaying: isPlaying,
      configuration: configuration
    )
  }
  
  private struct Representable: UIViewRepresentable {
    var name: String
    var bundle: Bundle
    var isPlaying: Bool
    var configuration: Configuration
    
    func makeUIView(context: Context) -> AnimationView {
      AnimationView(name: name, bundle: bundle)
    }
    
    func updateUIView(_ uiView: AnimationView, context: Context) {
      if configuration.resizable, #unavailable(iOS 16.0) {
        uiView.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
        uiView.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
      }
      uiView.loopMode = configuration.loopMode
      if isPlaying {
        if !uiView.isAnimationPlaying {
          uiView.play()
        }
      } else {
        uiView.stop()
      }
    }
    
    @available(iOS 16.0, *)
    func sizeThatFits(_ proposal: ProposedViewSize, uiView: AnimationView, context: Context) -> CGSize? {
      if configuration.resizable {
        return proposal.replacingUnspecifiedDimensions()
      }
      return uiView.intrinsicContentSize
    }
  }
}

extension LottieAnimationView {
  /// Sets the sizing mode for the Lottie animation to be resizable similar to
  /// `Image.resizable(capInsets:resizingMode:)`
  public func resizable() -> LottieAnimationView {
    var view = self
    view.configuration.resizable = true
    return view
  }
  
  /// Sets the loop mode of the Lottie animation
  public func loopMode(_ loopMode: LottieLoopMode) -> LottieAnimationView {
    var view = self
    view.configuration.loopMode = loopMode
    return view
  }
}
