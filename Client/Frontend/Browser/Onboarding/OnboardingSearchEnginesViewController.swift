// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import Shared

private let log = Logger.browserLogger

class OnboardingSearchEnginesViewController: OnboardingViewController {
    
    private struct UX {
        static let spaceBetweenRows: CGFloat = 8
    }
    
    var searchEngines: SearchEngines {
        profile.searchEngines
    }
    
    private lazy var availableEngines: [OpenSearchEngine] = {
        SearchEngines.getUnorderedBundledEnginesFor(locale: Locale.current)
    }()
    
    private var contentView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View(theme: theme)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        contentView.searchEnginesTable.dataSource = self
        contentView.searchEnginesTable.delegate = self
        
        contentView.continueButton.addTarget(self, action: #selector(continueTapped), for: .touchUpInside)
        contentView.skipButton.addTarget(self, action: #selector(skipTapped), for: .touchUpInside)
        
        contentView.searchEnginesTable.register(SearchEngineCell.self, forCellReuseIdentifier: String(describing: SearchEngineCell.self))
        
        let tablebackground = UIView()
        tablebackground.backgroundColor = OnboardingViewController.colorForTheme(theme)
        contentView.searchEnginesTable.backgroundView = tablebackground
        
        // This selection should only ever happen once.
        // We initially select the default search engine's cell
        DispatchQueue.main.async {
            for searchEngine in self.availableEngines.enumerated() {
                if searchEngine.element.shortName == self.searchEngines.defaultEngine().shortName {
                    let indexPath = IndexPath(row: 0, section: searchEngine.offset)
                    self.contentView.searchEnginesTable.selectRow(at: indexPath, animated: false,
                                                                  scrollPosition: .none)
                    break
                }
            }
        }
    }
    
    @objc override func continueTapped() {
        guard let selectedRow = contentView.searchEnginesTable.indexPathForSelectedRow?.section,
            let selectedEngine = availableEngines[safe: selectedRow]?.shortName else {
                return
            log.error("Failed to unwrap selected row or selected engine.")
        }
        
        searchEngines.setInitialDefaultEngine(selectedEngine)
        
        delegate?.presentNextScreen(current: self)
    }
    
    override func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        contentView.applyTheme(theme)
    }
}

extension OnboardingSearchEnginesViewController: UITableViewDelegate {
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return SearchEngineCell.preferredHeight
    }
    
    func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        return UX.spaceBetweenRows
    }
    
    func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let headerView = UIView()
        headerView.backgroundColor = UIColor.clear
        return headerView
    }
}

extension OnboardingSearchEnginesViewController: UITableViewDataSource {
    
    // Sections are used for data instead of rows.
    // This gives us an easy way to add spacing between each row.
    func numberOfSections(in tableView: UITableView) -> Int {
        return availableEngines.count
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return 1
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let cell = tableView.dequeueReusableCell(withIdentifier: String(describing: SearchEngineCell.self), for: indexPath) as? SearchEngineCell else {
            return SearchEngineCell()
        }
        
        guard let searchEngine = availableEngines[safe: indexPath.section] else {
            log.error("Can't find search engine at index: \(indexPath.section)")
            assertionFailure()
            return cell
        }
        
        cell.searchEngineName = searchEngine.shortName
        cell.searchEngineImage = searchEngine.image
        cell.selectedBackgroundColor = dark ? UIColor(rgb: 0x495057) : UIColor(rgb: 0xF0F2FF)
        cell.textLabel?.textColor = dark ? UIColor.lightText : UIColor.darkText
        cell.backgroundColor = .clear
        return cell
    }
}
