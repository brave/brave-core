// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Vision

class AIChatImageRecognition {
  private enum TextRecognitionError: Error {
    case invalidRequest
    case invalidImage
  }

  static func parseText(image: UIImage) async throws -> [String] {
    guard let image = image.cgImage else {
      throw TextRecognitionError.invalidImage
    }

    return try await withCheckedThrowingContinuation { continuation in
      let request = VNRecognizeTextRequest(completionHandler: { request, error in
        guard let request = request as? VNRecognizeTextRequest else {
          continuation.resume(throwing: error ?? TextRecognitionError.invalidRequest)
          return
        }

        var recognizedText = [String]()

        let observations = request.results ?? []
        for observation in observations {
          if let text = observation.topCandidates(1).first {
            recognizedText.append(text.string)
          }
        }

        continuation.resume(returning: recognizedText)
      })

      request.recognitionLevel = .accurate
      request.usesLanguageCorrection = true
      request.recognitionLanguages = [
        "en-US", "zh-Hans",
        "zh-Hant", "pt-BR",
        "fr-FR", "it-IT",
        "de-DE", "es-ES",
      ]
      request.automaticallyDetectsLanguage = true

      do {
        let requestHandler = VNImageRequestHandler(cgImage: image, options: [:])
        try requestHandler.perform([request])
      } catch {
        continuation.resume(throwing: error)
      }
    }
  }
}
