// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import SDWebImage
import SDWebImageSVGNativeCoder

private class WalletWebImageManager: ObservableObject {
  /// loaded image, note when progressive loading, this will published multiple times with different partial image
  @Published public var image: UIImage?
  /// loaded image data, may be nil if hit from memory cache. This will only published once loading is finished
  @Published public var isFinished: Bool = false
  /// loading error
  @Published public var error: Error?

  private var manager = SDWebImageManager.shared
  private var operation: SDWebImageOperation?

  private var supportedCoders: [SDImageCoder] = [SDImageSVGNativeCoder.shared, SDImageAPNGCoder.shared, SDImageGIFCoder.shared]
  
  init() {}

  func load(url: URL?, options: SDWebImageOptions = []) {
    operation = manager.loadImage(
      with: url, options: options, progress: nil,
      completed: { [weak self] image, data, error, _, finished, _ in
        guard let self = self else { return }
        self.image = image
        self.error = error
        if finished {
          self.isFinished = true
        }
      })
  }

  func cancel() {
    operation?.cancel()
    operation = nil
  }
  
  func load(base64Str: String, options: [SDImageCoderOption: Any] = [:]) {
    guard base64Str.hasPrefix("data:image/") else { return }
    guard let dataString = base64Str.separatedBy(",").last else { return }
    
    let data = Data(base64Encoded: dataString, options: .ignoreUnknownCharacters)
    for coder in supportedCoders where coder.canDecode(from: data) {
      image = coder.decodedImage(with: data, options: options)
      break
    }
  }
}

public struct WebImageReader<Content: View>: View {
  @StateObject private var imageManager: WalletWebImageManager = .init()
  var url: URL?
  var options: SDWebImageOptions
  var coderOptions: [SDImageCoderOption: Any]

  private var content: (_ image: UIImage?, _ isFinished: Bool) -> Content

  public init(
    url: URL?,
    options: SDWebImageOptions = [],
    coderOptions: [SDImageCoderOption: Any] = [:],
    @ViewBuilder content: @escaping (_ image: UIImage?, _ isFinished: Bool) -> Content
  ) {
    self.content = content
    self.url = url
    self.options = options
    self.coderOptions = coderOptions
  }

  public var body: some View {
    content(imageManager.image, imageManager.isFinished)
      .onAppear {
        if let urlString = url?.absoluteString {
          if urlString.hasPrefix("data:image/") {
            imageManager.load(base64Str: urlString)
          } else if !imageManager.isFinished {
              imageManager.load(url: url, options: options)
          }
        }
      }
  }
}
