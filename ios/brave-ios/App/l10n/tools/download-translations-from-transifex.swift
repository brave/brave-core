#!/usr/bin/swift

import Foundation

guard let apiKey = ProcessInfo.processInfo.environment["PASSWORD"] else {
  print("PASSWORD environment variable must be set to your Transifex API token")
  exit(EXIT_FAILURE)
}

struct LanguageMapping {
  /// The language code on the server.
  let source: String
  /// Sometimes we may want to have one language code to be used in many languages or dialects.
  /// This array has a list of those additional languages.
  /// For example German may have Swiss-German variant as their additional target.
  /// This list is empty if there is no additional targets.
  let additionalTargets: [String]

  /// Returns base language code and all additional targets we want to support.
  var targets: [String] {
    [source] + additionalTargets
  }

  init(source: String, additionalTargets: [String] = []) {
    self.source = source
    self.additionalTargets = additionalTargets
  }

  static var mappingsToDownload: [LanguageMapping] = {
    let arguments = CommandLine.arguments.dropFirst()  // First argument is the script name
    let providedLocales = Array(arguments)

    let mappings: [LanguageMapping] = [
      LanguageMapping(source: "fr"),
      LanguageMapping(source: "pl"),
      LanguageMapping(source: "ru"),
      LanguageMapping(source: "de", additionalTargets: ["gsw"]),
      LanguageMapping(source: "zh"),
      LanguageMapping(source: "zh_TW"),
      LanguageMapping(source: "id_ID"),
      LanguageMapping(source: "it"),
      LanguageMapping(source: "ja"),
      LanguageMapping(source: "ko_KR"),
      LanguageMapping(source: "ms"),
      LanguageMapping(source: "pt_BR"),
      LanguageMapping(source: "pt_PT"),
      LanguageMapping(source: "pt"),
      LanguageMapping(source: "es"),
      LanguageMapping(source: "uk"),
      LanguageMapping(source: "nb"),
      LanguageMapping(source: "sv"),
      LanguageMapping(source: "tr"),
      LanguageMapping(source: "ca"),
      LanguageMapping(source: "nl"),
      LanguageMapping(source: "cs"),
      LanguageMapping(source: "da"),
      LanguageMapping(source: "el"),
      LanguageMapping(source: "fi"),
      LanguageMapping(source: "hr", additionalTargets: ["bs"]),
      LanguageMapping(source: "hu"),
      LanguageMapping(source: "sk"),
      LanguageMapping(source: "ro"),
      LanguageMapping(source: "ar"),
      LanguageMapping(source: "he"),
      LanguageMapping(source: "th"),
      LanguageMapping(source: "vi"),
    ]

    if providedLocales.isEmpty {
      return mappings
    } else {
      let foundMappings = mappings.filter { providedLocales.contains($0.source) }
      let foundLocales = Set(foundMappings.map { $0.source })
      let notFoundLocales = providedLocales.filter { !foundLocales.contains($0) }

      notFoundLocales.forEach {
        printYellowWarning("WARNING: Locale '\($0)' not found in mappings")
      }

      return foundMappings
    }
  }()
}

func printYellowWarning(_ message: String) {
  print("\u{001B}[0;33m\(message)\u{001B}[0m")
}

@Sendable func validateResponse(_ response: HTTPURLResponse) {
  if response.statusCode == 401 {
    print("ERROR: Unauthorized access")
    exit(EXIT_FAILURE)
  }

  if response.statusCode != 200 && response.statusCode != 202 {
    print("ERROR: HTTP Status Code \(response.statusCode)")
    exit(EXIT_FAILURE)
  }
}

@Sendable func fetchResourceURL(languageCode code: String) async -> URL {
  var request = URLRequest(
    url: URL(string: "https://rest.api.transifex.com/resource_translations_async_downloads")!
  )
  request.setValue("Bearer \(apiKey)", forHTTPHeaderField: "Authorization")
  request.setValue("application/vnd.api+json", forHTTPHeaderField: "Content-Type")
  request.httpMethod = "POST"
  request.httpBody = """
    {
      "data": {
        "attributes": {
          "callback_url": null,
          "content_encoding": "text",
          "file_type": "default",
          "mode": "default",
          "pseudo": false
        },
        "relationships": {
          "language": {
            "data": {
              "id": "l:\(code)",
              "type": "languages"
            }
          },
          "resource": {
            "data": {
              "id": "o:brave:p:brave-ios:r:bravexliff",
              "type": "resources"
            }
          }
        },
        "type": "resource_translations_async_downloads"
      }
    }
    """.data(using: .utf8)!
  do {
    let (_, response) = try await URLSession.shared.data(for: request)
    guard let response = response as? HTTPURLResponse else { fatalError() }
    validateResponse(response)

    guard let contentLocation = response.value(forHTTPHeaderField: "Content-Location") as String?,
      let url = URL(string: contentLocation)
    else {
      print("ERROR: No content location found")
      exit(EXIT_FAILURE)
    }
    return url
  } catch {
    print("ERROR: Failed to download translations for \(code): \(String(describing: error))")
    exit(EXIT_FAILURE)
  }
}

@Sendable func downloadResource(with resourceURL: URL) async -> Data {
  var request = URLRequest(url: resourceURL)
  request.setValue("Bearer \(apiKey)", forHTTPHeaderField: "Authorization")
  request.httpMethod = "GET"
  do {
    let (data, response) = try await URLSession.shared.data(for: request)
    guard let response = response as? HTTPURLResponse else { fatalError() }
    validateResponse(response)
    return data
  } catch {
    print("ERROR: Failed to download resource at \(resourceURL): \(String(describing: error))")
    exit(EXIT_FAILURE)
  }
}

@Sendable func cleanupAndWriteResourceToDisk(xliffData: Data, languageCodes: [String]) async {
  let fileManager = FileManager.default
  for languageCode in languageCodes {
    let xliffURL = fileManager.temporaryDirectory.appendingPathComponent("\(languageCode).xliff")

    do {
      let dataToWrite: Data
      if languageCodes.count > 1 {
        // Update the target-language only if there are additional target languages
        dataToWrite = try updateTargetLanguage(xliffData: xliffData, newLanguage: languageCode)
      } else {
        dataToWrite = xliffData
      }
      try dataToWrite.write(to: xliffURL)
    } catch {
      print("ERROR: Failed to write or modify xliff file for \(languageCode): \(error)")
      exit(EXIT_FAILURE)
    }

    // Running an external Python script for XLIFF cleanup
    let process = Process()
    process.executableURL = URL(fileURLWithPath: "/usr/bin/env")
    process.arguments = ["python3", "xliff-cleanup.py", xliffURL.path]
    do {
      try process.run()
      process.waitUntilExit()
    } catch {
      print("Failed to run python process to cleanup xliff for \(languageCode): \(error)")
    }

    // Move the cleaned up XLIFF to a permanent directory
    do {
      let outputPath = URL(fileURLWithPath: fileManager.currentDirectoryPath)
        .appendingPathComponent("translated-xliffs")
      if !fileManager.fileExists(atPath: outputPath.path) {
        try fileManager.createDirectory(at: outputPath, withIntermediateDirectories: true)
      }
      try fileManager.moveItem(
        at: xliffURL,
        to: outputPath.appendingPathComponent("\(languageCode).xliff")
      )
    } catch {
      print("ERROR: Failed to move xliff file for \(languageCode): \(error)")
    }
  }
}

/// Replaces target language attribute in xliff to a new language.
/// This is required for languages which are not on the server but we want to support them locally.
/// The server has a base language xliff which we then 'copy' to another language,
/// only changing the target-language attr.
@Sendable func updateTargetLanguage(xliffData: Data, newLanguage: String) throws -> Data {
  let xmlDoc = try XMLDocument(data: xliffData, options: .nodePreserveWhitespace)
  let fileNodes = try xmlDoc.nodes(forXPath: "//file")

  for case let fileNode as XMLElement in fileNodes {
    if let targetLanguage = fileNode.attribute(forName: "target-language") {
      targetLanguage.stringValue = newLanguage
    }
  }

  return xmlDoc.xmlData(options: .nodePrettyPrint)
}

let resourceURLs: [String: URL] = await {
  var urls: [String: URL] = [:]
  // Prepare files for each language first
  for mapping in LanguageMapping.mappingsToDownload {
    if Task.isCancelled {
      exit(EXIT_FAILURE)
    }
    print("Preparing translations download for \(mapping.source)")
    urls[mapping.source] = await fetchResourceURL(languageCode: mapping.source)
  }
  return urls
}()

// Then attempt to download each one
try await withThrowingTaskGroup(of: Void.self) { group in
  for code in LanguageMapping.mappingsToDownload.map(\.source) {
    guard let resourceURL = resourceURLs[code] else {
      return
    }
    group.addTask {
      if Task.isCancelled {
        exit(EXIT_FAILURE)
      }
      var retryCount: Int = 0
      while true {
        let xliffData = await downloadResource(with: resourceURL)
        do {
          _ = try JSONSerialization.jsonObject(with: xliffData, options: [])
          // Found JSON, not ready
          retryCount += 1
          print(
            "Resource for \(code) not ready yet. Retrying in 5 seconds (attempt #\(retryCount))"
          )
          try await Task.sleep(nanoseconds: NSEC_PER_SEC * 5)
        } catch {
          // One language on the sever may may to multiple languages/dialects on the client.
          let targets =
            LanguageMapping.mappingsToDownload.first { $0.source == code }?.targets ?? [code]
          await cleanupAndWriteResourceToDisk(xliffData: xliffData, languageCodes: targets)
          print("Downloaded translations for \(code)")
          break
        }
        if retryCount > 5 {
          print("ERROR: Failed to download translations for \(code)")
          exit(EXIT_FAILURE)
        }
      }
    }
  }
  try await group.waitForAll()
}
