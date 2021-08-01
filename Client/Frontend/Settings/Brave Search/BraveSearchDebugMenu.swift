// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

struct BraveSearchDebugMenu: View {
    
    @ObservedObject var logging: BraveSearchLogEntry
    
    var body: some View {
        List {
            Section {
                let toggle = Toggle("Enable callback logging", isOn: $logging.isEnabled)
                if #available(iOS 14.0, *) {
                    toggle
                        .toggleStyle(SwitchToggleStyle(tint: .accentColor))
                } else {
                    toggle
                }
            }
            
            Section(header: Text(verbatim: "Logs")) {
                ForEach(logging.logs) { logEntry in
                    NavigationLink(destination: BraveSearchDebugMenuDetail(logEntry: logEntry)) {
                        VStack(alignment: .leading) {
                            Text(formattedDate(logEntry.date))
                                .font(.caption)
                            Text(logEntry.query)
                                .font(.body)
                        }
                    }
                }
            }
        }
    }
    
    private func formattedDate(_ date: Date) -> String {
        let dateFormatter = DateFormatter()
        dateFormatter.dateStyle = .short
        dateFormatter.timeStyle = .short
        return dateFormatter.string(from: date)
    }
}

#if DEBUG
struct BraveSearchDebugMenu_Previews: PreviewProvider {
    static var previews: some View {
        BraveSearchDebugMenu(logging: BraveSearchDebugMenuFixture.loggingSample)
    }
}
#endif
