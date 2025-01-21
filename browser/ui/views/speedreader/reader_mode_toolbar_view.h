/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_

#include <memory>

#include "base/observer_list.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"

class Browser;

class ReaderModeToolbarView : public views::View {
  METADATA_HEADER(ReaderModeToolbarView, views::View)
 public:
  struct Observer : public base::CheckedObserver {
    ~Observer() override = default;

    virtual void OnReaderModeToolbarActive(ReaderModeToolbarView* toolbar) {}
  };

  explicit ReaderModeToolbarView(Browser* browser);
  ~ReaderModeToolbarView() override;

  ReaderModeToolbarView(const ReaderModeToolbarView&) = delete;
  ReaderModeToolbarView(ReaderModeToolbarView&&) = delete;
  ReaderModeToolbarView& operator=(const ReaderModeToolbarView&) = delete;
  ReaderModeToolbarView& operator=(ReaderModeToolbarView&&) = delete;

  void SetVisible(bool visible) override;

  content::WebContents* GetWebContentsForTesting();

  views::View* toolbar() const { return toolbar_.get(); }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void NotifyActive();

 private:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  bool OnMousePressed(const ui::MouseEvent& event) override;

  std::unique_ptr<views::WebView> toolbar_;
  bool toolbar_loaded_ = false;

  base::ObserverList<Observer> observers_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
