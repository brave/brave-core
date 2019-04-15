/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

class SearchEnginePicker: UITableViewController {
    weak var delegate: SearchEnginePickerDelegate?
    var engines: [OpenSearchEngine]!
    var selectedSearchEngineName: String?
    
    var type: DefaultEngineType?
    
    convenience init(type: DefaultEngineType) {
        self.init(style: .plain)
        self.type = type
    }
    
    override init(style: UITableView.Style) {
        super.init(style: style)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()

        navigationItem.title = Strings.SearchEnginePickerNavTitle
        navigationItem.leftBarButtonItem = UIBarButtonItem(title: Strings.CancelButtonTitle, style: .plain, target: self, action: #selector(cancel))
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return engines.count
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let engine = engines[indexPath.item]
        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        cell.textLabel?.text = engine.shortName
        cell.imageView?.image = engine.image.createScaled(CGSize(width: OpenSearchEngine.PreferredIconSize, height: OpenSearchEngine.PreferredIconSize))
        if engine.shortName == selectedSearchEngineName {
            cell.accessoryType = .checkmark
        }
        return cell
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let engine = engines[indexPath.item]
        delegate?.searchEnginePicker(self, didSelectSearchEngine: engine, forType: type)
        tableView.cellForRow(at: indexPath)?.accessoryType = .checkmark
    }
    
    override func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
        tableView.cellForRow(at: indexPath)?.accessoryType = .none
    }
    
    @objc func cancel() {
        delegate?.searchEnginePicker(self, didSelectSearchEngine: nil, forType: nil)
    }
}
