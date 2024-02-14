/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
@testable import Storage
import UIKit
import XCTest
import TestHelpers

extension Sequence {
  fileprivate func asyncForEach(_ operation: (Element) async throws -> Void) async rethrows {
    for element in self {
      try await operation(element)
    }
  }
}

class DiskImageStoreTests: XCTestCase {
  var files: FileAccessor!
  var store: DiskImageStore!

  override func setUp() {
    files = MockFiles()
    store = try! DiskImageStore(files: files, namespace: "DiskImageStoreTests", quality: 1)

    Task { @MainActor in
      await store.clearExcluding(Set())
    }
  }

  @MainActor func testStore() async throws {
    // Avoid image comparison and use size of the image for equality
    let redImage = makeImageWithColor(UIColor.red, size: CGSize(width: 100, height: 100))
    let blueImage = makeImageWithColor(UIColor.blue, size: CGSize(width: 17, height: 17))

    try await [(key: "blue", image: blueImage), (key: "red", image: redImage)].asyncForEach { (key, image) in
      await XCTAssertAsyncThrowsError(try await store.get(key), "\(key) key is nil")
      await XCTAssertAsyncNoThrow(try await store.put(key, image: image), "\(key) image added to store")
      let storedImage = try! await store.get(key)
      XCTAssertEqual(storedImage.size.width, image.size.width, "Images are equal")
      await XCTAssertAsyncThrowsError(try await store.put(key, image: image), "\(key) image not added again")
    }

    await store.clearExcluding(Set(["red"]))
    await XCTAssertAsyncNoThrow(try await store.get("red"), "Red image still exists")
    await XCTAssertAsyncThrowsError(try await store.get("blue"), "Blue image cleared")
  }

  private func makeImageWithColor(_ color: UIColor, size: CGSize) -> UIImage {
    let rect = CGRect(size: size)
    UIGraphicsBeginImageContextWithOptions(size, false, 1.0)
    color.setFill()
    UIRectFill(rect)
    let image = UIGraphicsGetImageFromCurrentImageContext()!
    UIGraphicsEndImageContext()
    return image
  }
}
