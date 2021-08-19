// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import CoreData
import OrderedCollections

// MARK: - HistoryV2FetchResultsDelegate

protocol HistoryV2FetchResultsDelegate: AnyObject {
    
    func controllerWillChangeContent(_ controller: HistoryV2FetchResultsController)
    
    func controllerDidChangeContent(_ controller: HistoryV2FetchResultsController)
    
    func controller(_ controller: HistoryV2FetchResultsController, didChange anObject: Any,
                    at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?)
    
    func controller(_ controller: HistoryV2FetchResultsController, didChange sectionInfo: NSFetchedResultsSectionInfo,
                    atSectionIndex sectionIndex: Int, for type: NSFetchedResultsChangeType)
    
    func controllerDidReloadContents(_ controller: HistoryV2FetchResultsController)
}

// MARK: - HistoryV2FetchResultsController

protocol HistoryV2FetchResultsController {
    
    var delegate: HistoryV2FetchResultsDelegate? { get set }
    
    var fetchedObjects: [Historyv2]? { get }
    
    var fetchedObjectsCount: Int { get }
    
    var sectionCount: Int { get }
    
    func performFetch(_ completion: @escaping () -> Void)
    
    func object(at indexPath: IndexPath) -> Historyv2?
    
    func objectCount(for section: Int) -> Int
    
    func titleHeader(for section: Int) -> String

}

// MARK: - Historyv2Fetcher

class Historyv2Fetcher: NSObject, HistoryV2FetchResultsController {
    
    // MARK: Lifecycle
    
    init(historyAPI: BraveHistoryAPI) {
        self.historyAPI = historyAPI
        super.init()
        
        self.historyServiceListener = historyAPI.add(HistoryServiceStateObserver { [weak self] _ in
            guard let self = self else { return }
            
            DispatchQueue.main.async {
                self.delegate?.controllerDidReloadContents(self)
            }
        })
    }
    
    // MARK: Internal
    
    weak var delegate: HistoryV2FetchResultsDelegate?
    
    var fetchedObjects: [Historyv2]? {
        historyList
    }
    
    var fetchedObjectsCount: Int {
        historyList.count
    }
    
    var sectionCount: Int {
        return sectionDetails.elements.filter { $0.value > 0 }.count
    }
    
    func performFetch(_ completion: @escaping () -> Void) {
        clearHistoryData()
        
        historyAPI?.search(withQuery: "", maxCount: 200, completion: { [weak self] historyNodeList in
            guard let self = self else { return }
            
            self.historyList = historyNodeList.map { [unowned self] historyNode in
                let historyItem = Historyv2(with: historyNode)
                
                if let section = historyItem.sectionID, let numOfItemInSection = self.sectionDetails[section] {
                    self.sectionDetails.updateValue(numOfItemInSection + 1, forKey: section)
                }
                
                return historyItem
            }
            
            completion()
        })
    }
    
    func object(at indexPath: IndexPath) -> Historyv2? {
        let filteredDetails = sectionDetails.elements.filter { $0.value > 0 }
        var totalItemIndex = 0
        
        for sectionIndex in 0..<indexPath.section {
            totalItemIndex += filteredDetails[safe: sectionIndex]?.value ?? 0
        }

        return fetchedObjects?[safe: totalItemIndex + indexPath.row]
    }
    
    func objectCount(for section: Int) -> Int {
        let filteredDetails = sectionDetails.elements.filter { $0.value > 0 }
        return filteredDetails[safe: section]?.value ?? 0
    }
    
    func titleHeader(for section: Int) -> String {
        let filteredDetails = sectionDetails.elements.filter { $0.value > 0 }
        return  filteredDetails[safe: section]?.key.title ?? ""
    }
    
    // MARK: Private
    
    private var historyServiceListener: HistoryServiceListener?

    private weak var historyAPI: BraveHistoryAPI?
    
    private var historyList = [Historyv2]()
    
    private var sectionDetails: OrderedDictionary<Historyv2.Section, Int> = [.today: 0,
                                                                             .yesterday: 0,
                                                                             .lastWeek: 0,
                                                                             .thisMonth: 0]
    
    private func clearHistoryData() {
        historyList.removeAll()

        for key in sectionDetails.keys {
            sectionDetails.updateValue(0, forKey: key)
        }
    }
    
}
