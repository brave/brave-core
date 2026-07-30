import BraveCore
internal import CxxImports
internal import brave_ios_swift_bar

public class FooClass {
  private let bar: Bar
  public init() {
    self.bar = Bar()
    let cxxBuildNumber = base.swift.GetIOSBuildNumber()
    let buildNumber = String(cString: cxxBuildNumber.__c_strUnsafe())
    print(buildNumber)
  }
}

public struct FooStruct {
  var a: Int
}

public enum FooEnum {
  case one, two, three
}
