// Lookup time is O(maxDicts)
// Very basic implementation of a recent item collection class, stored as groups of items in dictionaries, oldest items are deleted as blocks of items since their entire containing dictionary is deleted.

import Foundation

class FifoDict<Key: Hashable, Element> {
  var fifoArrayOfDicts: [NSMutableDictionary] = []
  let maxDicts = 5
  let maxItemsPerDict = 50

  // the url key is a combination of urls, the main doc url, and the url being checked
  func addElement(_ element: Element?, forKey key: Key) {
    if fifoArrayOfDicts.count > maxItemsPerDict {
      fifoArrayOfDicts.removeFirst()
    }

    if fifoArrayOfDicts.last == nil || (fifoArrayOfDicts.last?.count ?? 0) > maxItemsPerDict {
      fifoArrayOfDicts.append(NSMutableDictionary())
    }

    if let lastDict = fifoArrayOfDicts.last {
      if element == nil {
        lastDict[key] = NSNull()
      } else {
        lastDict[key] = element
      }
    }
  }

  func getElement(_ key: Key) -> Element? {
    for dict in fifoArrayOfDicts {
      if let item = dict[key] {
        return item as? Element
      }
    }
    return nil
  }
}
