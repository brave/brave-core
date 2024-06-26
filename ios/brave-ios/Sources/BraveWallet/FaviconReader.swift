import BraveShared
import Favicon
import SwiftUI

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
      .task(id: url) {
        await load(url)
      }
  }

  private func load(_ url: URL) async {
    if let favicon = await FaviconFetcher.getIconFromCache(for: url) {
      self.image = favicon.image
      return
    }
    do {
      let favicon = try await FaviconFetcher.loadIcon(
        url: url,
        kind: .largeIcon,
        persistent: true
      )
      self.image = favicon.image
    } catch {
      self.image = nil
    }
  }
}
