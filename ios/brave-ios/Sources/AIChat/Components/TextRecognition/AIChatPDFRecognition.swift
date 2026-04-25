// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import PDFKit
import WebKit
import os.log

class AIChatPDFRecognition {
  static func parse(pdfData: String) async -> String? {
    guard let data = Data(base64Encoded: pdfData) else {
      return nil
    }

    guard let pdf = PDFDocument(data: data) else {
      return nil
    }

    let pageCount = pdf.pageCount
    let documentContent = NSMutableAttributedString()

    for i in 0..<pageCount {
      guard let page = pdf.page(at: i) else { continue }
      guard let pageContent = page.attributedString else { continue }
      documentContent.append(pageContent)
    }

    return documentContent.string
  }

  @MainActor
  static func parseToImage(pdfData: Data) async -> String? {
    guard let pdf = PDFDocument(data: pdfData) else {
      return nil
    }

    let pageCount = pdf.pageCount
    var images = [UIImage]()

    for i in 0..<pageCount {
      guard let page = pdf.page(at: i) else { continue }

      // Get Page Render Bounds
      let rect = page.bounds(for: .mediaBox)

      // Render to Image - High Resolution (2x)
      let image = page.thumbnail(
        of: CGSize(width: rect.width * 2.0, height: rect.height * 2.0),
        for: .mediaBox
      )

      images.append(image)
    }

    var unsortedTexts = await withTaskGroup(of: (index: Int, text: String).self) { group in
      for (index, image) in images.enumerated() {
        group.addTask {
          do {
            let text = try await AIChatImageRecognition.parseText(image: image).map({
              $0.trimmingCharacters(in: .whitespacesAndNewlines)
            }).joined(separator: " ")

            return (index, text)
          } catch {
            return (index, "")
          }
        }
      }

      var pairs = [(index: Int, text: String)]()
      for await pair in group {
        pairs.append(pair)
      }

      return pairs
    }

    unsortedTexts.sort { $0.index < $1.index }
    return unsortedTexts.map({ $0.text }).joined(separator: " ").trimmingCharacters(
      in: .whitespacesAndNewlines
    )
  }
}
