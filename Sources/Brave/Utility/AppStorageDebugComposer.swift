// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import os.log

struct AppStorageDebugComposer {
  
  /// This function prepares data to help us identify any app storage problems users may have.
  static func compose() -> String {
    var printDeviceInfo: String {
      let device = UIDevice.current
      let model = device.userInterfaceIdiom == .pad ? "iPad" : "iPhone"
      let iOSVersion = "\(device.systemName) \(device.systemVersion)"
      let appVersion = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "-"
      
      return "Setup: \(model), \(iOSVersion), Brave \(appVersion)"
    }
    
    var printFolderTreeStructure: String {
        let fm = FileManager.default
        guard let enumerator = fm.enumerator(
          at: URL(fileURLWithPath: NSHomeDirectory()),
          includingPropertiesForKeys: nil
        ) else { return "" }
        
        let formatter = ByteCountFormatter().then {
          $0.countStyle = .file
          $0.allowsNonnumericFormatting = false
          $0.allowedUnits = [.useKB, .useMB, .useGB, .useTB]
        }
        
        var result = ""
        
        while let file = enumerator.nextObject() as? URL {
          do {
            
            let isDirectory = (try file.resourceValues(forKeys: [.isDirectoryKey])).isDirectory == true
            // Skip individual files, skip folders below 4th level of nesting.
            guard isDirectory, enumerator.level <= 4 else { continue }

            let _1MB = 1000 * 1000
            // Skip folders smaller than 1MB
            guard let directorySize = try fm.directorySize(at: file), directorySize > _1MB else { continue }
            
            let formattedSize = formatter.string(fromByteCount: Int64(directorySize))
            
            let indentation = String(repeating: "\t", count: enumerator.level - 1)
            result += indentation + file.lastPathComponent + "(\(formattedSize))\n"
          } catch {
            Logger.module.error("AppStorageDebug error: \(error)")
            continue
          }
        }
        
        return result
      }
    
    let result =
    """
    \(printDeviceInfo)
    \(printFolderTreeStructure)
    """
    
    return result
  }
}
