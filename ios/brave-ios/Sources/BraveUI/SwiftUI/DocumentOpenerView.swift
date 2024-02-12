/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import UniformTypeIdentifiers

/// A representable to show a `UIDocumentPickerViewController` in open file mode
///
/// SwiftUI's `fileImporter` modifier has some strange bugs so this is a more manual
/// approach, but at least works properly
///
/// - note: Changing `allowedContentTypes` will do nothing once the picker has been presented
public struct DocumentOpenerView: UIViewControllerRepresentable {
  public let allowedContentTypes: [UTType]
  public var allowsMultipleSelection: Bool
  public var initialDirectoryURL: URL?
  public var onCompletion: ([URL]) -> Void

  public init(
    allowedContentTypes: [UTType],
    allowsMultipleSelection: Bool = false,
    initialDirectoryURL: URL? = nil,
    onCompletion: @escaping ([URL]) -> Void
  ) {
    self.allowedContentTypes = allowedContentTypes
    self.allowsMultipleSelection = allowsMultipleSelection
    self.initialDirectoryURL = initialDirectoryURL
    self.onCompletion = onCompletion
  }

  public typealias UIViewControllerType = UIDocumentPickerViewController
  public func makeUIViewController(context: Context) -> UIViewControllerType {
    let picker = UIDocumentPickerViewController(forOpeningContentTypes: allowedContentTypes)
    picker.delegate = context.coordinator
    return picker
  }
  public func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
    uiViewController.allowsMultipleSelection = allowsMultipleSelection
    uiViewController.directoryURL = initialDirectoryURL
  }
  public func makeCoordinator() -> Coordinator {
    Coordinator(onCompletion)
  }
  public class Coordinator: NSObject, UIDocumentPickerDelegate {
    var onCompletion: ([URL]) -> Void
    init(_ completion: @escaping ([URL]) -> Void) {
      onCompletion = completion
    }
    public func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
      DispatchQueue.main.async {
        self.onCompletion(urls)
      }
    }
  }
}
