// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

private struct XorshiftRandomNumberGenerator: RandomNumberGenerator {
  var x: Int32
  var y: Int32
  var z: Int32
  var w: Int32

  mutating func next() -> UInt64 {
    let t = x ^ (x << 11)
    x = y
    y = z
    z = w
    w = (w ^ (w >> 19)) ^ (t ^ (t >> 8))
    return UInt64(w)
  }
}

class Blockies {
  struct Colors {
    let color: UIColor
    let backgroundColor: UIColor
    let spotColor: UIColor
  }

  private var generator: XorshiftRandomNumberGenerator
  private let colors: [Int] = [
    0x423EEE, 0xE2E2FC, 0xFE5907, 0xFEDED6, 0x5F5CF1,
    0x171553, 0x1C1E26, 0xE1E2E8,
  ]

  init(seed: String) {
    let startIndex = seed.startIndex
    var xorSeed: [Int32] = [0, 0, 0, 0]
    for i in 0..<seed.count {
      let index = i % 4
      let a: Int32 = xorSeed[index] &* (2 << 4)
      let b: Int32 = xorSeed[index]
      let c: Int32 = Int32(seed[seed.index(startIndex, offsetBy: i)].asciiValue ?? 0)
      xorSeed[index] = (a &- b &+ c)
    }
    self.generator = .init(
      x: xorSeed[0],
      y: xorSeed[1],
      z: xorSeed[2],
      w: xorSeed[3]
    )
  }

  func makeColors() -> Colors {
    .init(color: makeColor(), backgroundColor: makeColor(), spotColor: makeColor())
  }

  func makeColor() -> UIColor {
    let normalized = Double(generator.next()) / Double(Int32.max)
    return UIColor(rgb: colors[Int(floor(normalized * 100)) % colors.count])
  }

  func rand() -> Double {
    Double(generator.next()) / Double(Int32.max)
  }

  func image(length: Int, scale: CGFloat) -> UIImage {
    let colors = makeColors()

    func data() -> [Double] {
      let dataLength = Int(ceil(Double(length) / 2.0))
      var data: [[Double]] = []
      for _ in 0..<length {
        var row: [Double] = []
        for _ in 0..<dataLength {
          let normalized = Double((generator.next() >> 0)) / Double(Int32.max)
          let skewed = floor(normalized * 2.3)
          row.append(skewed)
        }
        let mirrorCopy = row.reversed()
        data.append(row + mirrorCopy)
      }
      return data.flatMap { $0 }
    }

    let size = CGSize(width: CGFloat(length) * scale, height: CGFloat(length) * scale)
    let renderer = UIGraphicsImageRenderer(size: size)
    let data = data()
    let width = sqrt(Double(data.count))

    let image = renderer.image { context in
      colors.backgroundColor.setFill()
      context.fill(.init(origin: .zero, size: size))
      colors.spotColor.setFill()
      for (i, value) in data.enumerated() where value > 0 {
        let row = floor(Double(i) / width)
        let col = i % Int(width)
        let fillColor = value == 1 ? colors.color : colors.spotColor
        let shapeType = floor(rand() * 3)

        switch shapeType {
        case 0:
          let rectSizeMultiplier = rand() * 2
          let rect = CGRect(
            x: Int(col) * Int(scale),
            y: Int(row) * Int(scale),
            width: Int(scale * rectSizeMultiplier),
            height: Int(scale * rectSizeMultiplier)
          )
          fillColor.setFill()
          context.fill(rect)
        case 1:
          let rectSizeMultiplier = rand()
          let x = Int(col) * Int(scale) + Int(scale) / 2 - Int(scale * rectSizeMultiplier / 2)
          let y = Int(row) * Int(scale) + Int(scale) / 2 - Int(scale * rectSizeMultiplier / 2)
          let rect = CGRect(
            x: x,
            y: y,
            width: Int(scale * rectSizeMultiplier),
            height: Int(scale * rectSizeMultiplier)
          )
          fillColor.setFill()
          context.cgContext.fillEllipse(in: rect)
        default:
          break
        }
      }
    }

    return image
  }
}

struct Blockie: View {
  enum Shape {
    case circle
    case rectangle
  }

  var address: String
  var shape: Shape = .rectangle

  private var base: some View {
    Image(uiImage: Blockies(seed: address.lowercased()).image(length: 4, scale: 25))
      .resizable()
  }

  var body: some View {
    if shape == .circle {
      base
        .clipShape(Circle())
    } else {
      base
        .clipShape(RoundedRectangle(cornerRadius: 4))
    }
  }
}

struct BlockieBackground: View {

  let colors: Blockies.Colors

  init(seed: String) {
    self.colors = Blockies(seed: seed).makeColors()
  }

  var body: some View {
    LinearGradient(
      colors: [.init(colors.backgroundColor), .init(colors.spotColor)],
      startPoint: .top,
      endPoint: .bottom
    )
  }
}

#if DEBUG
struct BlockiesPreview: PreviewProvider {
  static var previews: some View {
    ZStack {
      Blockie(address: "45a36a8e118c37e4c47ef4ab827a7c9e579e11e2")
        .frame(width: 64, height: 64)
        .overlay(
          Blockie(address: "25a36a8e118c37e4c47ef4ab827a7c9e579e11e2")
            .frame(width: 32, height: 32)
            .offset(x: 32, y: 0)
        )
    }
    .padding()
    .padding()
    .previewLayout(.sizeThatFits)
  }
}
#endif
