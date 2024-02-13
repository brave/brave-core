import SwiftUI
import BraveShared
import Favicon

struct FaviconReader<Content: View>: View {
  let url: URL
  @State private var image: UIImage?
  private var content: (_ image: UIImage?) -> Content
  @State private var faviconTask: Task<Void, Error>?

  init(
    url: URL,
    @ViewBuilder content: @escaping (_ image: UIImage?) -> Content
  ) {
    self.url = url
    self.content = content
  }
  
  var body: some View {
    content(image)
      .onAppear {
        load(url, transaction: Transaction())
      }
      .onChange(of: url) { newValue in
        load(newValue, transaction: Transaction())
      }
  }
  
  private func load(_ url: URL?, transaction: Transaction) {
    guard let url = url else { return }
    faviconTask?.cancel()
    if let favicon = FaviconFetcher.getIconFromCache(for: url) {
      faviconTask = nil
      
      withTransaction(transaction) {
        self.image = favicon.image
      }
      return
    }
    
    faviconTask = Task { @MainActor in
      do {
        let favicon = try await FaviconFetcher.loadIcon(url: url, kind: .largeIcon, persistent: true)
        withTransaction(transaction) {
          self.image = favicon.image
        }
      } catch {
        withTransaction(transaction) {
          self.image = nil
        }
      }
    }
  }
}
