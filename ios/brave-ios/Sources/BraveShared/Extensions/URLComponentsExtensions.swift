import Foundation

extension URLComponents {
  // Return the first query parameter that matches
  public func valueForQuery(_ param: String) -> String? {
    return self.queryItems?.first { $0.name == param }?.value
  }
}
