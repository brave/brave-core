// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Shared
import Preferences
import Data
import CoreData
import BraveUI

extension Domain: Identifiable {
  public var id: String {
    objectID.uriRepresentation().absoluteString
  }
}

private extension Domain {
  var host: String {
    URL(string: url ?? "")?.host ?? url ?? ""
  }
}

struct PageZoomSettingsView: View {
  private let context = DataController.swiftUIContext
  
  @State private var domains = PageZoomSettingsView.filteredDomains(DataController.swiftUIContext)
  
  @State private var defaultZoomLevel = Preferences.General.defaultPageZoomLevel.value
  
  var body: some View {
    List {
      if !domains.isEmpty {
        Section(
          header: Text(Strings.PageZoom.specificWebsitesZoomLevelsSectionTitle)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
        ) {
          ForEach(domains) { domain in
            HStack {
              let url = domain.host
              let zoomLevel = domain.zoom_level?.doubleValue ?? 1.0
              
              Text(url)
                .font(.system(.body).weight(.medium))
                .foregroundColor(Color(.bravePrimary))
              
              Spacer()
              
              Text(NSNumber(value: zoomLevel), formatter: PageZoomView.percentFormatter)
                .font(.system(.body).weight(.medium))
                .foregroundColor(Color(.bravePrimary))
                .multilineTextAlignment(.trailing)
            }
          }
          .onDelete(perform: resetItemZoomLevel)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      
      Section(
        header: Text(domains.isEmpty ? Strings.PageZoom.defaultZoomLevelSectionTitle : Strings.PageZoom.otherWebsiteZoomLevelSectionTitle)
          .font(.subheadline)
          .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        ForEach(PageZoomHandler.steps, id: \.self) { step in
          Button(action: {
            resetDefaultZoomLevel(zoomLevel: step)
          }, label: {
            HStack(spacing: 0.0) {
              Text(NSNumber(value: step), formatter: PageZoomView.percentFormatter)
                .font(.system(.body).weight(.medium))
                .foregroundColor(Color(.bravePrimary))
                .multilineTextAlignment(.trailing)
              
              Spacer()
              
              let zoomLevel = Int(step * 100.0)
              let preferredZoomLevel = Int(defaultZoomLevel * 100.0)
              if preferredZoomLevel == zoomLevel {
                Image(systemName: "checkmark")
                  .font(.system(.body).weight(.medium))
                  .foregroundColor(Color(.braveBlurpleTint))
              }
            }
          })
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      
      if !domains.isEmpty {
        Section(
          header: Text(Strings.PageZoom.resetAllDescription)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
        ) {
          Button(action: {
            PageZoomSettingsView.allDomains(context).forEach({
              $0.zoom_level = nil
            })
            
            try? context.save()
            
            domains = PageZoomSettingsView.filteredDomains(context)
            NotificationCenter.default.post(name: PageZoomView.notificationName, object: nil)
          }, label: {
            Text(Strings.PageZoom.resetAll)
              .font(.system(.body).weight(.medium))
          })
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .listStyle(.insetGrouped)
  }
  
  // When this view `deinit`, if a zoom level is the same as the general zoom level,
  // we set it to `nil` just like in Safari Desktop.
  func normalizeZoomLevels() {
    domains.forEach({
      if $0.zoom_level?.doubleValue == Preferences.General.defaultPageZoomLevel.value {
        $0.zoom_level = nil
      }
    })
    
    try? context.save()
    NotificationCenter.default.post(name: PageZoomView.notificationName, object: nil)
  }
  
  private func resetDefaultZoomLevel(zoomLevel: Double) {
    Preferences.General.defaultPageZoomLevel.value = zoomLevel
    defaultZoomLevel = zoomLevel
    
    domains = PageZoomSettingsView.filteredDomains(context)
  }
  
  private func resetItemZoomLevel(at offsets: IndexSet) {
    offsets.forEach({
      domains[$0].zoom_level = nil
    })
    
    try? context.save()
    
    domains = PageZoomSettingsView.filteredDomains(context)
    NotificationCenter.default.post(name: PageZoomView.notificationName, object: nil)
  }
  
  private static func allDomains(_ context: NSManagedObjectContext) -> [Domain] {
    let request = NSFetchRequest<Domain>().then {
      $0.entity = Domain.entity(context)
      $0.sortDescriptors = [NSSortDescriptor(key: "url", ascending: true)]
    }
    
    do {
      return try context.fetch(request)
    } catch {
      return []
    }
  }
  
  private static func filteredDomains(_ context: NSManagedObjectContext) -> [Domain] {
    let request = NSFetchRequest<Domain>().then {
      $0.entity = Domain.entity(context)
      $0.sortDescriptors = [NSSortDescriptor(key: "url", ascending: true)]
      $0.predicate = NSPredicate(format: "zoom_level != nil")
    }
    
    do {
      // Domain.url is a string unfortunately
      // So all sorting by `host` and filtering by `scheme` has to be done in Swift rather than as a CoreData predicate
      return try context.fetch(request).filter({
        if let urlString = $0.url, let url = URL(string: urlString) {
          return url.scheme == "https"
        }
        return false
      }).sorted(by: { $0.host < $1.host })
    } catch {
      return []
    }
  }
}

#if DEBUG
struct PageZoomSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    PageZoomSettingsView()
  }
}
#endif

class PageZoomSettingsController: UIHostingController<PageZoomSettingsView> {
  
  init() {
    super.init(rootView: PageZoomSettingsView())
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  deinit {
    rootView.normalizeZoomLevels()
  }
}
