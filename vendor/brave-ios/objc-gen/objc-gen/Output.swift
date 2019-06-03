/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

protocol TemplateOutput {
  /// The file this template will be written too
  var filename: String { get }
  /// The generated string for this template
  var generated: String { get }
}

let thisFileIsGeneratedString = """
/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
"""

/// Enums.h
final class EnumHeaderOutput: TemplateOutput {
  let enums: [Enum]
  init(enums: [Enum]) {
    self.enums = enums
  }
  var filename: String {
    return "Enums.h"
  }
  var generated: String {
    return """
    \(thisFileIsGeneratedString)
    
    #import <Foundation/Foundation.h>
    
    \(enums.map { $0.generated }.joined(separator: "\n\n"))
    
    """
  }
}

/// Records.h
final class RecordsHeaderOutput: TemplateOutput {
  let interfaces: [Interface]
  init(interfaces: [Interface]) {
    self.interfaces = interfaces
  }
  var filename: String {
    return "Records.h"
  }
  var generated: String {
    return  """
    \(thisFileIsGeneratedString)
    
    #import <Foundation/Foundation.h>
    #import "Enums.h"
    
    @class \(interfaces.map { $0.prefixedName }.joined(separator: ", "));
    
    NS_ASSUME_NONNULL_BEGIN
    
    \(interfaces.map { $0.generatedPublicInterface }.joined(separator: "\n\n"))
    
    NS_ASSUME_NONNULL_END
    
    """
  }
}

/// Records+Private.h
final class PrivateRecordsHeaderOutput: TemplateOutput {
  let interfaces: [Interface]
  let cppIncludes: [String]
  init(interfaces: [Interface], cppIncludes: [String]) {
    self.interfaces = interfaces
    self.cppIncludes = cppIncludes
  }
  var filename: String {
    return "Records+Private.h"
  }
  var generated: String {
    return  """
    \(thisFileIsGeneratedString)
    
    #import <Foundation/Foundation.h>
    #import "Records.h"
    
    \(cppIncludes.map { "#include \"\($0)\"" }.joined(separator: "\n"))
    
    \(interfaces.map { $0.generatedPrivateInterface }.joined(separator: "\n\n"))
    
    """
  }
}

/// Records.mm
final class ImplementationSourceOutput: TemplateOutput {
  let interfaces: [Interface]
  init(interfaces: [Interface]) {
    self.interfaces = interfaces
  }
  var filename: String {
    return "Records.mm"
  }
  var generated: String {
    return """
    \(thisFileIsGeneratedString)
    
    #import "Records.h"
    #import "Records+Private.h"
    #import "CppTransformations.h"
    
    #import <vector>
    #import <map>
    #import <string>
    
    \(interfaces.map { $0.generatedImplementation }.joined(separator: "\n\n"))
    
    """
  }
}
