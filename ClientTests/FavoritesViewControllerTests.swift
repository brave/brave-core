// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Data
@testable import Client

class FavoritesViewControllerTests: XCTestCase {

    var dataSource: MockFavoritesDataSource!
    var delegate: MockTopSitesDelegate!
    var vc: FavoritesViewController!
    var collectionView: UICollectionView!

    override func setUp() {
        super.setUp()

        delegate = MockTopSitesDelegate()
        dataSource = MockFavoritesDataSource()
        vc = FavoritesViewController(profile: MockProfile(), dataSource: dataSource, fromOverlay: false, rewards: nil, backgroundDataSource: nil)
        vc.delegate = delegate
        collectionView = UICollectionView(frame: CGRect.zero, collectionViewLayout: UICollectionViewFlowLayout())
    }

    override func tearDown() {
        dataSource = nil
        delegate = nil
        vc = nil
        collectionView = nil
        
        super.tearDown()
    }

    func testFavoritesViewControllerLoadsView() {
        let viewController =
            FavoritesViewController(profile: MockProfile(),
                                    dataSource: MockFavoritesDataSource(),
                                    fromOverlay: false,
                                    rewards: nil,
                                    backgroundDataSource: nil)
        XCTAssertNotNil(viewController.view, "Unable to load view")
        XCTAssertNotNil(viewController.view.subviews.first { $0 is UICollectionView }, "`UICollectionView` missing from `FavoritesViewController` view.")
        XCTAssertNil(viewController.delegate)
    }

    func testTopSiteDelegate_ReceivesString() {
        let index = IndexPath(item: 0, section: 0)
        let I_LOVE_CHOCOLATE = "I Love Chocolate"
        dataSource.bookmarks[index] = createBookmark(I_LOVE_CHOCOLATE)

        // Firing `UICollectionView` delegate method
        vc.collectionView(collectionView, didSelectItemAt: index)
        
        XCTAssertEqual(delegate.input, I_LOVE_CHOCOLATE, "The Favorites destination is incorrect.")
        XCTAssertFalse(delegate.isReturningURL, "Favorites should work for any string NOT just URL's.")
    }

    func testTopSiteDelegate_ReceivesFullURL() {
        let index = IndexPath(item: 0, section: 0)
        let HTTPS_BRAVE_COM = "https://www.brave.com"
        dataSource.bookmarks[index] = createBookmark(HTTPS_BRAVE_COM)

        vc.collectionView(collectionView, didSelectItemAt: index)

        XCTAssertEqual(delegate.input, HTTPS_BRAVE_COM, "The Favorites destination is incorrect.")
        XCTAssertTrue(delegate.isReturningURL, "Favorites should work for URL's.")
    }

    func testTopSiteDelegate_ReceivesURLWithoutScheme() {
        let index = IndexPath(item: 0, section: 0)
        let BRAVE_COM = "brave.com"
        dataSource.bookmarks[index] = createBookmark(BRAVE_COM)

        vc.collectionView(collectionView, didSelectItemAt: index)

        XCTAssertEqual(delegate.input, BRAVE_COM, "The Favorites destination is incorrect.")
        XCTAssertTrue(delegate.isReturningURL, "Favorites should work for URL's without a scheme.")
    }

    func testTopSiteDelegate_EmptyString() {
        let index = IndexPath(item: 0, section: 0)
        let EMPTY_STRING = ""
        dataSource.bookmarks[index] = createBookmark(EMPTY_STRING)

        vc.collectionView(collectionView, didSelectItemAt: index)

        XCTAssertEqual(delegate.input, EMPTY_STRING, "The Favorites destination is incorrect.")
        XCTAssertTrue(delegate.didSelectInputString)
        XCTAssertFalse(delegate.isReturningURL)
    }

    func testTopSiteDelegate_NilURL() {
        let index = IndexPath(item: 0, section: 0)
        let NIL_URL: String? = nil
        dataSource.bookmarks[index] = createBookmark(NIL_URL)

        vc.collectionView(collectionView, didSelectItemAt: index)

        XCTAssertEqual(delegate.input, NIL_URL, "The Favorites destination is incorrect.")
        XCTAssertFalse(delegate.didSelectInputString)
    }

    fileprivate func createBookmark(_ urlString: String? = nil) -> Bookmark? {
        return Bookmark(context: DataController.viewContext).then {
            $0.url = urlString
        }
    }
}

class MockFavoritesDataSource: FavoritesDataSource {

    var bookmarks = [IndexPath: Bookmark]()

    var _isEditing: Bool = false
    override var isEditing: Bool {
        get { return _isEditing }
        set { _isEditing = newValue }
    }

    override init() {
    }

    override func favoriteBookmark(at indexPath: IndexPath) -> Bookmark? {
        return bookmarks[indexPath]
    }
}

class MockTopSitesDelegate: FavoritesDelegate {

    var didSelectInputString = false
    var didTapCallout = false
    var input: String?

    func didSelect(input: String) {
        didSelectInputString = true
        self.input = input
    }

    func didTapDuckDuckGoCallout() {
        didTapCallout = true
    }

    /// Checks that there is a `urlString` AND is it able to be created into a `URL`.
    var isReturningURL: Bool {
        guard let input = self.input else { return false }
        return URL(string: input) != nil
    }
    
    func didTapShowMoreFavorites() {
        // Protocol conformance
    }
    
    func openBrandedImageCallout(state: BrandedImageCalloutState?) {
        // Protocol conformance
    }
}
