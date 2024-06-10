// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Lottie
import UIKit

extension AnimationView {
  static func showShredAnimation(on view: UIView, callback: @escaping () -> Void) {
    let animationView = AnimationView(name: "shred", bundle: .module)
    animationView.contentMode = .scaleAspectFill
    animationView.loopMode = .playOnce
    animationView.animationSpeed = 1
    animationView.frame = view.bounds
    view.addSubview(animationView)
    animationView.play()

    Task {
      do {
        try await Task.sleep(seconds: 0.6)
      } catch {
        // Ignore
      }

      callback()
      UIView.animate(withDuration: 0.4) {
        animationView.alpha = 0
      } completion: { _ in
        animationView.removeFromSuperview()
      }
    }
  }
}
