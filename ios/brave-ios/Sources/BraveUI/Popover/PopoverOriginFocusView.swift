// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SwiftUI

/// The background highlight view that animates to help the user focus on the
/// popovers origin.
class PopoverOriginFocusView: UIView {
  let rings: [UIView] = (Int(0)..<3).map { index in
    let view = UIView()
    view.backgroundColor = .white.withAlphaComponent(1.0 - (CGFloat(index) * 0.1))
    return view
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    rings.reversed().forEach(addSubview)
    clipsToBounds = false
    isUserInteractionEnabled = false
    
    // For the animation to "pulse" the first ring almost
    rings.first?.alpha = 0.75
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    for ring in rings {
      // bounds + center since we're doing transform animations below
      ring.bounds = bounds
      ring.center = bounds.center
      ring.layer.cornerRadius = bounds.width/2
    }
  }
  
  private var animator: UIViewPropertyAnimator?
  
  func beginAnimating() {
    if let animator, animator.isRunning { return }
    for ring in rings.dropFirst() {
      ring.layer.removeAllAnimations()
      ring.transform = .identity
      ring.alpha = 1
    }
    let animator = UIViewPropertyAnimator(duration: 2, dampingRatio: 1.0)
    animator.addAnimations {
      for (index, ring) in self.rings.enumerated() {
        let scale: CGFloat = 1 + CGFloat(index) / 2.5
        ring.transform = .init(scaleX: scale, y: scale)
      }
      for ring in self.rings.dropFirst() {
        ring.alpha = 0
      }
    }
    animator.addCompletion { _ in
      self.animator = nil
      self.beginAnimating()
    }
    animator.startAnimation()
    self.animator = animator
  }
  
  func stopAnimating() {
    animator?.stopAnimation(false)
    animator = nil
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

#if DEBUG
struct PopoverOriginFocusView_PreviewProvider: PreviewProvider {
  struct Representable: UIViewRepresentable {
    func makeUIView(context: Context) -> PopoverOriginFocusView {
      PopoverOriginFocusView()
    }
    func updateUIView(_ uiView: PopoverOriginFocusView, context: Context) {
      uiView.beginAnimating()
    }
    @available(iOS 16.0, *)
    func sizeThatFits(_ proposal: ProposedViewSize, uiView: PopoverOriginFocusView, context: Context) -> CGSize? {
      return proposal.replacingUnspecifiedDimensions()
    }
  }
  static var previews: some View {
    Image(braveSystemName: "leo.brave.icon-monochrome")
      .foregroundStyle(LinearGradient(braveGradient: .lightGradient02))
      .imageScale(.large)
      .padding(8)
      .background {
        Representable()
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color.gray)
  }
}
#endif
