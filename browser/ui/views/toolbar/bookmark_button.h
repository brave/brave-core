/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BOOKMARK_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BOOKMARK_BUTTON_H_

#include "base/scoped_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class BookmarkButton : public ToolbarButton {
 public:
    explicit BookmarkButton(views::ButtonListener* listener);
    ~BookmarkButton() override;

    void SetToggled(bool on);
    void UpdateImage();

    // Invoked when a bubble for this icon is created. The Button
    // changes highlights based on this widget's visibility.
    void OnBubbleWidgetCreated(views::Widget* bubble_widget);

    // ToolbarButton:
    base::string16 GetTooltipText(const gfx::Point& p) const override;
    const char* GetClassName() const override;
    void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
    bool active_ = false;
    // Highlights the ink drop for the icon, used when the corresponding widget
    // is visible.
    void SetHighlighted(bool bubble_visible);

    class WidgetObserver : public views::WidgetObserver {
     public:
        explicit WidgetObserver(BookmarkButton* parent);
        ~WidgetObserver() override;

        void SetWidget(views::Widget* widget);

     private:
        // views::WidgetObserver:
        void OnWidgetDestroying(views::Widget* widget) override;
        void OnWidgetVisibilityChanged(views::Widget* widget,
                                      bool visible) override;

        BookmarkButton* const parent_;
        ScopedObserver<views::Widget, views::WidgetObserver> scoped_observer_;
        DISALLOW_COPY_AND_ASSIGN(WidgetObserver);
    };
    WidgetObserver widget_observer_;

    DISALLOW_COPY_AND_ASSIGN(BookmarkButton);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BOOKMARK_BUTTON_H_
