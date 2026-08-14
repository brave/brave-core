internal import CxxImports
import Foundation

public class Bar {
  public init() {
    let ts = timespec(tv_sec: 2, tv_nsec: 0)
    let timeDelta: base.swift.TimeDelta = base.swift.TimeDelta.FromTimeSpec(ts)
  }
}
