import SwiftUI

struct Server: Identifiable, Equatable {
  let id = UUID()
  let name: String
  var isAutomatic = false
}

class ServerDetail: ObservableObject {
  var servers = [
    Server(name: "Automatic", isAutomatic: true),
    Server(name: "São Paulo"),
    Server(name: "Rio de Janeiro"),
    Server(name: "Brasília"),
    Server(name: "Fortaleza"),
  ]

  @Published var selectedServer: Server? = nil

  init(isAutoSelectEnabled: Bool = true) {
    selectedServer = servers.first
  }
}

struct BraveRegionDetailsView: View {

  @State private var isAutoSelectEnabled: Bool
  @State private var isLoading = false
  @ObservedObject private var serverDetail = ServerDetail()

  public init(isAutoSelectEnabled: Bool = false, serverDetail: ServerDetail? = nil) {
    self.isAutoSelectEnabled = isAutoSelectEnabled

    if let serverDetail = serverDetail {
      self.serverDetail = serverDetail
    }
  }

  var body: some View {
    ZStack {
      List {
        Section(header: Text("AVAILABLE SERVERS")) {
          ForEach(serverDetail.servers) { server in
            HStack {
              VStack(alignment: .leading) {
                Text(server.name)
                  .foregroundColor(serverDetail.selectedServer == server ? .blue : .black)

                if server.isAutomatic == true {
                  Text("Use the best server available")
                    .foregroundColor(serverDetail.selectedServer == server ? .blue : .black)
                }
              }
              Spacer()

              if serverDetail.selectedServer == server {
                Image(systemName: "checkmark")
                  .foregroundColor(.blue)
              }
            }
            .contentShape(Rectangle())
            .onTapGesture {
              selectDesignatedVPNServer(server)

            }
          }
        }
      }

      if isLoading {
        BraveVPNRegionLoadingIndicatorView()
          .transition(.opacity)
          .zIndex(1)
      }
    }
    .navigationBarTitle("Brazil Server", displayMode: .inline)
  }

  private func selectDesignatedVPNServer(_ server: Server) {
    guard !isLoading else {
      return
    }

    isLoading = true

    // TODO: Select Region
    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
      serverDetail.selectedServer = server
      isLoading = false
    }
  }

}

struct ServerViewModel_Previews: PreviewProvider {
  static var previews: some View {
    BraveRegionDetailsView()
  }
}
