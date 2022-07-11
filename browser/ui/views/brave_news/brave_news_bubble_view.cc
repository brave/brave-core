// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

// static
base::WeakPtr<views::Widget> BraveNewsBubbleView::Show(
    views::View* anchor,
    content::WebContents* contents) {
  auto* widget = views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<BraveNewsBubbleView>(anchor, contents));
  widget->Show();
  return widget->GetWeakPtr();
}

class BraveNewsFeedRow : public views::View,
                         public BraveNewsTabHelper::PageFeedsObserver {
 public:
  explicit BraveNewsFeedRow(BraveNewsTabHelper::FeedDetails details,
                            content::WebContents* contents)
      : feed_details_(details), contents_(contents) {
    DCHECK(contents_);
    tab_helper_ = BraveNewsTabHelper::FromWebContents(contents);
    tab_helper_->AddObserver(this);

    auto* const layout =
        SetLayoutManager(std::make_unique<views::FlexLayout>());
    layout->SetOrientation(views::LayoutOrientation::kHorizontal);
    layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
    layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

    auto* title = AddChildView(
        std::make_unique<views::Label>(base::UTF8ToUTF16(details.title)));
    title->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
    title->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                                 views::MaximumFlexSizeRule::kUnbounded));

    subscribe_button_ = AddChildView(std::make_unique<views::MdTextButton>(
        base::BindRepeating(&BraveNewsFeedRow::OnPressed,
                            base::Unretained(this)),
        u""));

    Update();
  }

  ~BraveNewsFeedRow() override { tab_helper_->RemoveObserver(this); }

  void Update() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents_);
    if (!tab_helper) {
      return;
    }

    auto is_subscribed = tab_helper->is_subscribed(feed_details_);
    subscribe_button_->SetText(l10n_util::GetStringUTF16(
        loading_        ? IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_LOADING
        : is_subscribed ? IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_UNSUBSCRIBE
                        : IDS_BRAVE_NEWS_BUBBLE_FEED_ITEM_SUBSCRIBE));
    subscribe_button_->SetProminent(!is_subscribed && !loading_);
  }

  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override {
    loading_ = false;
    Update();
  }

  void OnPressed() {
    // Don't queue multiple toggles.
    if (loading_)
      return;

    tab_helper_->ToggleSubscription(feed_details_);
    loading_ = true;
    Update();
  }

 private:
  bool loading_ = false;
  raw_ptr<views::MdTextButton> subscribe_button_ = nullptr;

  BraveNewsTabHelper::FeedDetails feed_details_;
  raw_ptr<content::WebContents> contents_;
  raw_ptr<BraveNewsTabHelper> tab_helper_;
};

BraveNewsBubbleView::BraveNewsBubbleView(views::View* action_view,
                                         content::WebContents* contents)
    : views::BubbleDialogDelegateView(action_view,
                                      views::BubbleBorder::TOP_RIGHT,
                                      views::BubbleBorder::STANDARD_SHADOW),
      contents_(contents) {
  DCHECK(contents);

  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetAccessibleRole(ax::mojom::Role::kDialog);
  set_adjust_if_offscreen(true);

  title_label_ = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_BUBBLE_DISMISS_FOREVER),
      views::style::CONTEXT_DIALOG_TITLE));

  views::FlexLayout* const layout =
      SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);
  layout->SetCollapseMargins(true);

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
  for (const auto& feed_item : tab_helper->available_feeds()) {
    auto* child =
        AddChildView(std::make_unique<BraveNewsFeedRow>(feed_item, contents));
    child->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(10, 0, 0, 0));
  }

  auto* dismiss_button = AddChildView(std::make_unique<views::MdTextButton>(
      base::BindRepeating(&BraveNewsBubbleView::DismissForever,
                          base::Unretained(this)),
      u"Hide and don't show this again"));
  dismiss_button->SetProperty(views::kMarginsKey,
                              gfx::Insets::TLBR(10, 0, 0, 0));
  dismiss_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));
}

BraveNewsBubbleView::~BraveNewsBubbleView() = default;

void BraveNewsBubbleView::Update() {}

void BraveNewsBubbleView::DismissForever() {
  GetWidget()->Hide();
  auto* profile = Profile::FromBrowserContext(contents_->GetBrowserContext());
  profile->GetPrefs()->SetBoolean(brave_news::prefs::kShouldShowToolbarButton,
                                  false);
}

BEGIN_METADATA(BraveNewsBubbleView, views::BubbleDialogDelegateView)
END_METADATA
