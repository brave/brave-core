/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LOCATION_BAR_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LOCATION_BAR_H_

#include "base/scoped_observation.h"
#include "components/prefs/pref_member.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Label;
class ImageView;
}  // namespace views

class PrefService;

// A simple version of the location bar for secondary web view.
// This location bar only shows host of the current page.
//  e.g. http://foo.bar.com/1234 => foo.bar.com
// When the scheme is not https, the location bar will show an icon to indicate
// the site might not be safe.
class SplitViewLocationBar : public views::WidgetDelegateView,
                             public content::WebContentsObserver,
                             public views::ViewObserver {
  METADATA_HEADER(SplitViewLocationBar, views::WidgetDelegateView)

 public:
  ~SplitViewLocationBar() override;

  static SplitViewLocationBar* Create(PrefService* prefs,
                                      views::View* web_view);

  void SetWebContents(content::WebContents* web_contents);

  // views::WidgetDelegateView:
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaintBorder(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void DidChangeVisibleSecurityState() override;
  void WebContentsDestroyed() override;

  // views::ViewObserver:
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view) override;
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

 private:
  explicit SplitViewLocationBar(PrefService* service);

  void UpdateVisibility();
  void UpdateBounds();
  void UpdateURLAndIcon();
  void UpdateIcon();
  bool IsContentsSafe() const;
  bool HasCertError() const;
  url_formatter::FormatUrlType GetURLFormatType() const;

  SkPath GetBorderPath(bool close);

  raw_ptr<PrefService> prefs_ = nullptr;

  raw_ptr<views::ImageView> safety_icon_ = nullptr;
  raw_ptr<views::Label> https_with_strike_ = nullptr;
  raw_ptr<views::Label> scheme_separator_ = nullptr;
  raw_ptr<views::Label> url_ = nullptr;

  BooleanPrefMember prevent_url_elision_;

  base::ScopedObservation<views::View, views::ViewObserver> view_observation_{
      this};
};

BEGIN_VIEW_BUILDER(/*no export*/,
                   SplitViewLocationBar,
                   views::WidgetDelegateView)
END_VIEW_BUILDER

DEFINE_VIEW_BUILDER(/*no export*/, SplitViewLocationBar)

#endif  // __BRAVE_BROWSER_SRC_BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LOCATION_BAR_H_
