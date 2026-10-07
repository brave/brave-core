// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Strings

private class _WidgetBundleClass {}
private let widgetBundle = Bundle(for: _WidgetBundleClass.self)

// MARK: - Widgets
extension Strings {
  public struct Widgets {
    public static let noFavoritesFound = NSLocalizedString(
      "widgets.noFavoritesFound",
      bundle: widgetBundle,
      value: "Please open Brave to view your favorites here",
      comment: "This shows when you add a widget but have no favorites added in your app"
    )

    public static let favoritesWidgetDescription = NSLocalizedString(
      "widgets.favoritesWidgetDescription",
      bundle: widgetBundle,
      value: "Quickly access your favorite websites.",
      comment: "Description for favorites widget on 'add widget' screen."
    )

    public static let shortcutsWidgetTitle = NSLocalizedString(
      "widgets.shortcutsWidgetTitle",
      bundle: widgetBundle,
      value: "Shortcuts",
      comment: "Title for shortcuts widget on 'add widget' screen."
    )

    public static let shortcutsEmptyState = NSLocalizedString(
      "widgets.shortcutsEmptyState",
      bundle: widgetBundle,
      value: "Your shortcuts will appear here",
      comment: "A label shown on the widget has no available shortcuts to display."
    )

    public static let shortcutsWidgetDescription = NSLocalizedString(
      "widgets.shortcutsWidgetDescription",
      bundle: widgetBundle,
      value: "Quick access to search the web or open web pages in Brave.",
      comment: "Description for shortcuts widget on 'add widget' screen."
    )

    public static let shortcutsEnterURLButton = NSLocalizedString(
      "widgets.shortcutsEnterURLButton",
      bundle: widgetBundle,
      value: "Search or type a URL",
      comment: "Button to the browser and enter URL or make a search query there."
    )

    public static let shieldStatsTitle = NSLocalizedString(
      "widgets.shieldStatsTitle",
      bundle: widgetBundle,
      value: "Privacy Stats",
      comment: "Title for Brave Shields widget on 'add widget' screen."
    )

    public static let shieldStatsDescription = NSLocalizedString(
      "widgets.shieldStatsDescription",
      bundle: widgetBundle,
      value: "A summary of how Brave saves you time and protects you online.",
      comment: "Description for Brave Shields widget on 'add widget' screen."
    )

    public static let shieldStatsWidgetTitle = NSLocalizedString(
      "widgets.shieldStatsWidgetTitle",
      bundle: widgetBundle,
      value: "Privacy Stats",
      comment: "Title of Brave Shields widget shown above stat numbers."
    )

    public static let singleStatTitle = NSLocalizedString(
      "widgets.singleStatTitle",
      bundle: widgetBundle,
      value: "Privacy Stat",
      comment: "Title for Brave Shields single stat widget on 'add widget' screen."
    )

    public static let singleStatDescription = NSLocalizedString(
      "widgets.singleStatDescription",
      bundle: widgetBundle,
      value: "A summary of how Brave has protected you online.",
      comment: "Description for Brave Shields single stat widget on 'add widget' screen."
    )

    public static let newsClusteringWidgetTitle = NSLocalizedString(
      "widgets.newsClusteringWidgetTitle",
      bundle: widgetBundle,
      value: "Top News",
      comment: "Title for Brave News widgets that display the top news"
    )
    public static let newsClusteringWidgetDescription = NSLocalizedString(
      "widgets.newsClusteringWidgetDescription",
      bundle: widgetBundle,
      value: "Get the latest news",
      comment: "Description for Brave News widgets that display the top news"
    )
    public static let newsClusteringErrorLabel = NSLocalizedString(
      "widgets.newsClusteringErrorLabel",
      bundle: widgetBundle,
      value: "Try again later",
      comment: "Displayed on a news widget that has failed to load"
    )
    public static let newsClusteringReadMoreButtonTitle = NSLocalizedString(
      "widgets.newsClusteringReadMoreButtonTitle",
      bundle: widgetBundle,
      value: "Read More",
      comment: "Displayed on a news widget that links into the app to read more Brave News content"
    )
    public static let braveNews = NSLocalizedString(
      "widgets.braveNews",
      bundle: widgetBundle,
      value: "Brave News",
      comment: "The name of the feature"
    )
    public static let newsUnavailableByPolicy = NSLocalizedString(
      "widgets.newsUnavailableByPolicy",
      bundle: widgetBundle,
      value: "Brave News is Disabled",
      comment: "Displayed on a news widget wont load due to admin policies"
    )
    public static let newsUnavailableByPolicyDescription = NSLocalizedString(
      "widgets.newsUnavailableByPolicyDescription",
      bundle: widgetBundle,
      value: "Turned off by your organization's policy",
      comment: "Displayed on a news widget wont load due to admin policies"
    )
    public static let braveSearch = NSLocalizedString(
      "widgets.braveSearch",
      bundle: widgetBundle,
      value: "Brave Search",
      comment:
        "Title for the Brave Search control. Brave is the company name and should not be translated"
    )
    public static let braveSearchWidgetTitle = NSLocalizedString(
      "widgets.braveSearchWidgetTitle",
      bundle: widgetBundle,
      value: "Brave Search",
      comment:
        "Title for the Brave Search widget on 'add custom control' screen. Brave is the company name and should not be translated"
    )
    public static let braveSearchWidgetDescription = NSLocalizedString(
      "widgets.braveSearchWidgetDescription",
      bundle: widgetBundle,
      value: "Search the web with Brave.",
      comment:
        "Description for the Brave Search widget on 'add custom control' screen. Brave is the company name and should not be translated"
    )
    public static let askBraveWidgetTitle = NSLocalizedString(
      "widgets.askBraveWidgetTitle",
      bundle: widgetBundle,
      value: "Ask Brave",
      comment:
        "Title for the Ask Brave widget on 'add custom control' screen. Brave is the company name and should not be translated"
    )
    public static let askBraveWidgetDescription = NSLocalizedString(
      "widgets.askBraveWidgetDescription",
      bundle: widgetBundle,
      value: "Ask Brave a question.",
      comment:
        "Description for the Ask Brave widget on 'add custom control' screen. Brave is the company name and should not be translated"
    )
  }
}
