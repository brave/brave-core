/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI

private struct XorshiftRandomNumberGenerator: RandomNumberGenerator {
  var x, y, z, w: UInt32
  
  mutating func next() -> UInt64 {
    let t = x ^ (x << 11)
    x = y; y = z; z = w
    w = (w ^ (w >> 19)) ^ (t ^ (t >> 8))
    return UInt64(w)
  }
}

class Blockies {
  private var generator: XorshiftRandomNumberGenerator
  private let colors: [Int] = [
    0x5B5C63, 0x151E9A, 0x2197F9, 0x1FC3DC, 0x086582,
    0x67D4B4, 0xAFCE57, 0xF0CB44, 0xF28A29, 0xFC798F,
    0xC1226E, 0xFAB5EE, 0x9677EE, 0x5433B0,
  ]
  
  init(seed: String) {
    let startIndex = seed.startIndex
    var xorSeed: [UInt32] = [0, 0, 0, 0]
    for i in 0..<seed.count {
      let index = i % 4
      let a: UInt32 = xorSeed[index] &* (2 << 4)
      let b: UInt32 = xorSeed[index]
      let c: UInt32 = UInt32((seed[seed.index(startIndex, offsetBy: i)].asciiValue ?? 0))
      xorSeed[index] = (a &- b &+ c)
    }
    self.generator = .init(
      x: xorSeed[0],
      y: xorSeed[1],
      z: xorSeed[2],
      w: xorSeed[3]
    )
  }
  
  func makeColor() -> UIColor {
    let normalized = Double(generator.next()) / Double(UInt32.max)
    return UIColor(rgb: colors[Int(floor(normalized * 100)) % colors.count])
  }
  
  func image(length: Int, scale: CGFloat) -> UIImage {
    let color = makeColor()
    let backgroundColor = makeColor()
    let spotColor = makeColor()
    
    func data() -> [[Double]] {
      let dataLength = Int(ceil(Double(length) / 2.0))
      var data: [[Double]] = []
      for _ in 0..<length {
        var row: [Double] = []
        for _ in 0..<dataLength {
          let normalized = Double((generator.next() >> 0)) / Double(UInt32.max)
          let skewed = floor(normalized * 2.3)
          row.append(skewed)
        }
        let mirrorCopy = row.reversed()
        data.append(row + mirrorCopy)
      }
      return data
    }
    
    let size = CGSize(width: CGFloat(length) * scale, height: CGFloat(length) * scale)
    let renderer = UIGraphicsImageRenderer(size: size)
    let data = data()
    let image = renderer.image { context in
      backgroundColor.setFill()
      context.fill(.init(origin: .zero, size: size))
      spotColor.setFill()
      for (y, row) in data.enumerated() {
        for (x, value) in row.enumerated() {
          let rect = CGRect(x: x, y: y, width: 1, height: 1)
            .applying(CGAffineTransform(scaleX: scale, y: scale))
          if value > 0 {
            if value == 1 {
              color.setFill()
            } else {
              spotColor.setFill()
            }
            context.fill(rect)
          }
        }
      }
    }
    return image
  }
}

struct Blockie: View {
  var address: String
  
  var body: some View {
    Image(uiImage: Blockies(seed: address).image(length: 8, scale: 16))
      .resizable()
      .blur(radius: 8, opaque: true)
      .clipShape(Circle())
  }
}

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
