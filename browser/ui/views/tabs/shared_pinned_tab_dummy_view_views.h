/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_VIEWS_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_dummy_view.h"
#include "chrome/browser/ui/thumbnails/thumbnail_image.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

class ThumbnailImage;

class SharedPinnedTabDummyViewViews : public SharedPinnedTabDummyView,
                                      public views::View {
  METADATA_HEADER(SharedPinnedTabDummyViewViews, views::View)

 public:
  ~SharedPinnedTabDummyViewViews() override;

  // SharedPinnedTabDummyView:
  void Install() override;

 private:
  friend class SharedPinnedTabDummyView;

  // Use SharedPinnedTabDummyView::Create() instead.
  SharedPinnedTabDummyViewViews(content::WebContents* shared_contents,
                                content::WebContents* dummy_contents);

  raw_ptr<content::WebContents> shared_contents_;
  raw_ptr<content::WebContents> dummy_contents_;

  scoped_refptr<ThumbnailImage> thumbnail_;
  std::unique_ptr<ThumbnailImage::Subscription> subscription_;

  raw_ptr<views::ImageView> thumbnail_view_;

  raw_ptr<views::Label> title_label_;
  raw_ptr<views::Label> description_label_;
};

BEGIN_VIEW_BUILDER(/*no export*/, SharedPinnedTabDummyViewViews, views::View)
END_VIEW_BUILDER

DEFINE_VIEW_BUILDER(/*no export*/, SharedPinnedTabDummyViewViews)

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_VIEWS_H_
