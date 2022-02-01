// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import SDWebImage

private class WalletWebImageManager: ObservableObject {
  /// loaded image, note when progressive loading, this will published multiple times with different partial image
  @Published public var image: UIImage?
  /// loaded image data, may be nil if hit from memory cache. This will only published once loading is finished
  @Published public var isFinished: Bool = false
  /// loading error
  @Published public var error: Error?
  
  private var manager = SDWebImageManager.shared
  private var operation: SDWebImageOperation?
  
  init() { }
  
  func load(url: URL?, options: SDWebImageOptions = []) {
    operation = manager.loadImage(with: url, options: options, progress: nil, completed: { [weak self] image, data, error, _, finished, _ in
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
}

struct WebImageReader<Content: View>: View {
  @StateObject private var imageManager: WalletWebImageManager = .init()
  var url: URL?
  var options: SDWebImageOptions
  
  private var content: (_ image: UIImage?, _ isFinished: Bool) -> Content
  
  init(
    url: URL?,
    options: SDWebImageOptions = [],
    @ViewBuilder content: @escaping (_ image: UIImage?, _ isFinished: Bool) -> Content
  ) {
    self.content = content
    self.url = url
    self.options = options
  }
  
  var body: some View {
    content(imageManager.image, imageManager.isFinished)
      .onAppear {
        if !imageManager.isFinished {
          imageManager.load(url: url, options: options)
        }
      }
  }
}
