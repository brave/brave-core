import SwiftUI

struct Server: Identifiable, Equatable {
  let id = UUID()
  let name: String
  var isAutomatic = false
}

class ServerViewModel: ObservableObject {
  var servers = [
    Server(name: "Automatic", isAutomatic: true),
    Server(name: "São Paulo"),
    Server(name: "Rio de Janeiro"),
    Server(name: "Brasília"),
    Server(name: "Fortaleza"),
  ]

  @Published var selectedServer: Server? = nil

  init() {
    selectedServer = servers.first
  }
}

struct BraveRegionDetailsView: View {
  @StateObject private var viewModel = ServerViewModel()

  var body: some View {
    NavigationView {
      List {
        Section(header: Text("AVAILABLE SERVERS")) {
          ForEach(viewModel.servers) { server in
            HStack {
              VStack(alignment: .leading) {
                Text(server.name)
                  .foregroundColor(viewModel.selectedServer == server ? .blue : .black)

                if server.isAutomatic == true {
                  Text("Use the best server available")
                    .foregroundColor(viewModel.selectedServer == server ? .blue : .black)
                }
              }
              Spacer()

              if viewModel.selectedServer == server {
                Image(systemName: "checkmark")
                  .foregroundColor(.blue)
              }
            }
            .contentShape(Rectangle())
            .onTapGesture {
              viewModel.selectedServer = server
            }
          }
        }
      }
      .navigationBarTitle("Brazil Server", displayMode: .inline)
    }
  }
}

struct ServerViewModel_Previews: PreviewProvider {
  static var previews: some View {
    BraveRegionDetailsView()
  }
}
