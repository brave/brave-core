#!/usr/bin/swift

import Foundation

guard let apiKey = ProcessInfo.processInfo.environment["PASSWORD"] else {
  print("PASSWORD environment variable must be set to your Transifex API token")
  exit(EXIT_FAILURE)
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
          let url = URL(string: contentLocation) else {
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

@Sendable func cleanupAndWriteResourceToDisk(xliffData: Data, languageCode: String) async {
  let fileManager = FileManager.default
  let xliffURL = fileManager.temporaryDirectory.appendingPathComponent("\(languageCode).xliff")
  do {
    try xliffData.write(to: xliffURL)
  } catch {
    print("ERROR: Failed to write xliff file")
    exit(EXIT_FAILURE)
  }

  let process = Process()
  process.executableURL = URL(fileURLWithPath: "/usr/bin/env")
  process.arguments = ["python3", "xliff-cleanup.py", xliffURL.path]
  do {
    try process.run()
    process.waitUntilExit()
  } catch {
    print("Failed to run python process to cleanup xliff")
  }

  do {
    let outputPath = URL(fileURLWithPath: fileManager.currentDirectoryPath).appendingPathComponent("translated-xliffs")
    if !fileManager.fileExists(atPath: outputPath.path) {
      try fileManager.createDirectory(at: outputPath, withIntermediateDirectories: true)
    }
    try fileManager.moveItem(at: xliffURL, to: outputPath.appendingPathComponent("\(languageCode).xliff"))
  } catch {
    print("ERROR: Failed to move files: \(String(describing: error))")
  }
}

let languageCodes = [ "fr", "pl", "ru", "de", "zh", "zh_TW", "id_ID", "it", "ja", "ko_KR", "ms", "pt_BR", "es", "uk", "nb", "sv", "tr" ]
let resourceURLs: [String: URL] = await {
  var urls: [String: URL] = [:]
  // Prepare files for each language first
  for code in languageCodes {
    if Task.isCancelled {
      exit(EXIT_FAILURE)
    }
    print("Preparing translations download for \(code)")
    urls[code] = await fetchResourceURL(languageCode: code)
  }
  return urls
}()

// Then attempt to download each one
try await withThrowingTaskGroup(of: Void.self) { group in
  for code in languageCodes {
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
          print("Resource for \(code) not ready yet. Retrying in 5 seconds (attempt #\(retryCount))")
          try await Task.sleep(nanoseconds: NSEC_PER_SEC * 5)
        } catch {
          await cleanupAndWriteResourceToDisk(xliffData: xliffData, languageCode: code)
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
