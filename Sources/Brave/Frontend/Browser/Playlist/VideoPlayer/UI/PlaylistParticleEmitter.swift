// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

class PlaylistParticleEmitter: UIView {

  override init(frame: CGRect) {
    super.init(frame: frame)

    if let layer = self.layer as? CAEmitterLayer {
      let cells = generateEmitterCells()
      cells.base.emitterCells = [cells.trailing, cells.particle]
      layer.emitterCells = [cells.base]
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override class var layerClass: AnyClass {
    CAEmitterLayer.self
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    if let layer = self.layer as? CAEmitterLayer {
      layer.emitterSize = bounds.size
      layer.emitterPosition = CGPoint(x: bounds.width / 2, y: bounds.height / 2)
      layer.renderMode = .additive
    }
  }

  private func generateEmitterCells() -> (base: CAEmitterCell, trailing: CAEmitterCell, particle: CAEmitterCell) {
    let baseEmitter = CAEmitterCell().then {
      $0.color = #colorLiteral(red: 0.4980392157, green: 0.4980392157, blue: 0.4980392157, alpha: 0.5)
      $0.redRange = 0.9
      $0.greenRange = 0.9
      $0.blueRange = 0.9
      $0.lifetime = 2.5
      $0.birthRate = 5
      $0.velocity = 300
      $0.velocityRange = 100
      $0.emissionRange = .pi * 2
      $0.spinRange = .pi * 4
      $0.yAcceleration = 0
    }

    let trailingEmitter = CAEmitterCell().then {
      $0.contents = UIImage(named: "shields-menu-icon", in: .module, compatibleWith: nil)!.cgImage
      $0.lifetime = 0.5
      $0.birthRate = 45
      $0.velocity = 80
      $0.scale = 0.4
      $0.alphaSpeed = -0.7
      $0.scaleSpeed = -0.1
      $0.scaleRange = 0.1
      $0.beginTime = 0.01
      $0.duration = 1.7
      $0.emissionRange = .pi * 2
      $0.spinRange = .pi * 4
      $0.yAcceleration = 0
    }

    let particleEmitter = CAEmitterCell().then {
      $0.contents = UIImage(named: "shields-menu-icon", in: .module, compatibleWith: nil)!.cgImage
      $0.lifetime = 100
      $0.birthRate = 1000
      $0.velocity = 130
      $0.scale = 0.6
      $0.spin = 2
      $0.alphaSpeed = -0.2
      $0.scaleSpeed = -0.1
      $0.beginTime = 1.5
      $0.duration = 0.1
      $0.emissionRange = .pi * 2
      $0.spinRange = .pi * 4
      $0.yAcceleration = 0
    }
    return (baseEmitter, trailingEmitter, particleEmitter)
  }
}
