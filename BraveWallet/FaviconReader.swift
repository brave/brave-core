import SwiftUI

public protocol WalletFaviconRenderer {
  func loadIcon(siteURL: URL, persistent: Bool, completion: ((UIImage?) -> Void)?)
}

private class UnimplementedFaviconRenderer: WalletFaviconRenderer {
  public func loadIcon(siteURL: URL, persistent: Bool, completion: ((UIImage?) -> Void)?) {
    assertionFailure("FaviconRenderer not passed into environment, some favicons will fail to load.")
  }
}

struct FaviconRendererKey: EnvironmentKey {
  public static var defaultValue: WalletFaviconRenderer = UnimplementedFaviconRenderer()
}

extension EnvironmentValues {
  var faviconRenderer: WalletFaviconRenderer {
    get { self[FaviconRendererKey.self] }
    set { self[FaviconRendererKey.self] = newValue }
  }
}

struct FaviconReader<Content: View>: View {
  let url: URL
  @State private var image: UIImage?
  private var content: (_ image: UIImage?) -> Content
  @Environment(\.faviconRenderer) private var renderer: WalletFaviconRenderer

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
        load(url, transaction: Transaction(), using: renderer)
      }
      .onChange(of: url) { newValue in
        load(newValue, transaction: Transaction(), using: renderer)
      }
  }
  
  private func load(_ url: URL?, transaction: Transaction, using renderer: WalletFaviconRenderer) {
    guard let url = url else { return }
    renderer.loadIcon(siteURL: url, persistent: true) { image in
      withTransaction(transaction) {
        self.image = image
      }
    }
  }
}
