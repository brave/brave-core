/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Clang

final class NativeClientHeaderOutput: TemplateOutput {
  let namespace: String
  let className: String
  let methods: [Method]
  let includeHeader: String
  
  init(namespace: String, className: String, includeHeader: String, methods: [Method]) {
    self.namespace = namespace
    self.className = className
    self.includeHeader = includeHeader
    self.methods = methods
  }
  
  var filename: String {
    return "Native\(className).h"
  }
  var generated: String {
    let nativeClassName = "Native\(className)"
    let protocolName = "\(nativeClassName)Bridge"
    
    return """
    \(thisFileIsGeneratedString)
    
    #import <Foundation/Foundation.h>
    #import "\(includeHeader)"
    
    @protocol \(protocolName);
    
    class \(nativeClassName) : public \(namespace)::\(className) {
    public:
      \(nativeClassName)(id<\(protocolName)> bridge);
      ~\(nativeClassName)() override;
    
    private:
      __unsafe_unretained id<\(protocolName)> bridge_;
    
    \(methods.map { "  \($0.generatedPublicDecleration)" }.joined(separator: "\n"))
    };
    
    """
  }
}

final class NativeClientSourceOutput: TemplateOutput {
  let className: String
  let methods: [Method]
  
  init(className: String, methods: [Method]) {
    self.className = className
    self.methods = methods
  }
  
  var filename: String {
    return "Native\(className).mm"
  }
  var generated: String {
    let nativeClassName = "Native\(className)"
    let protocolName = "\(nativeClassName)Bridge"
    
    return """
    \(thisFileIsGeneratedString)
    
    #import "\(nativeClassName).h"
    #import "\(protocolName).h"
    
    // Constructor & Destructor
    \(nativeClassName)::\(nativeClassName)(id<\(protocolName)> bridge) : bridge_(bridge) { }
    \(nativeClassName)::~\(nativeClassName)() {
      bridge_ = nil;
    }
    
    \(methods.map { (method) -> String in
      let definition = method.generatedSourceImplementation(parentClass: nativeClassName)
      return """
      \(definition) {
        \(method.resultIsVoid ? "" : "return ")[bridge_ \(method.generatedProtocolMethodCall)];
      }
      """
    }.joined(separator: "\n"))
    
    """
  }
}

final class NativeClientBridgeProtocolOutput: TemplateOutput {
  let className: String
  let methods: [Method]
  let includeHeader: String
  
  init(className: String, includeHeader: String, methods: [Method]) {
    self.className = className
    self.includeHeader = includeHeader
    self.methods = methods
  }
  
  var filename: String {
    return "Native\(className)Bridge.h"
  }
  
  var generated: String {
    let nativeClassName = "Native\(className)"
    let protocolName = "\(nativeClassName)Bridge"
    
    return """
    \(thisFileIsGeneratedString)
    
    #import <Foundation/Foundation.h>
    #import "\(includeHeader)"
    
    @protocol \(protocolName)
    @required
    
    \(methods.map { $0.generatedProtocolMethodDecleration }.joined(separator: "\n"))
    
    @end
    
    """
  }
}
