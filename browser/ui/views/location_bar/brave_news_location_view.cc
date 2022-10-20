// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/location_bar/brave_news_location_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_member.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view.h"

namespace {
constexpr SkColor kSubscribedLightColor = SkColorSetRGB(76, 84, 210);
constexpr SkColor kSubscribedDarkColor = SkColorSetRGB(115, 122, 222);
}  // namespace

class BraveNewsButtonView : public views::LabelButton,
                            public TabStripModelObserver,
                            public BraveNewsTabHelper::PageFeedsObserver {
 public:
  BraveNewsButtonView(Profile* profile, TabStripModel* tab_strip)
      : views::LabelButton(
            base::BindRepeating(&BraveNewsButtonView::ButtonPressed,
                                base::Unretained(this))),
        profile_(profile),
        tab_strip_(tab_strip) {
    DCHECK(profile_);
    SetAccessibleName(
        l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP));
    SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_CENTER);

    auto* ink_drop = views::InkDrop::Get(this);
    ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
    ink_drop->SetBaseColorCallback(base::BindRepeating(
        [](views::View* host) { return GetToolbarInkDropBaseColor(host); },
        this));
    ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);
    SetHasInkDropActionOnClick(true);

    tab_strip_->AddObserver(this);
    if (tab_strip_->GetActiveWebContents()) {
      BraveNewsTabHelper::FromWebContents(tab_strip_->GetActiveWebContents())
          ->AddObserver(this);
    }

    should_show_.Init(brave_news::prefs::kShouldShowToolbarButton,
                      profile->GetPrefs(),
                      base::BindRepeating(&BraveNewsButtonView::Update,
                                          base::Unretained(this)));
    news_enabled_.Init(brave_news::prefs::kNewTabPageShowToday,
                       profile->GetPrefs(),
                       base::BindRepeating(&BraveNewsButtonView::Update,
                                           base::Unretained(this)));

    auto menu_button_controller = std::make_unique<views::MenuButtonController>(
        this,
        base::BindRepeating(&BraveNewsButtonView::ButtonPressed,
                            base::Unretained(this)),
        std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
    SetButtonController(std::move(menu_button_controller));

    Update();
  }
  ~BraveNewsButtonView() override = default;
  BraveNewsButtonView(const BraveNewsButtonView&) = delete;
  BraveNewsButtonView& operator=(const BraveNewsButtonView&) = delete;

  void Update() {
    if (!should_show_.GetValue() || !news_enabled_.GetValue()) {
      SetVisible(false);
      return;
    }

    auto* contents = tab_strip_->GetActiveWebContents();
    bool subscribed = false;
    bool has_feeds = false;
    absl::optional<BraveNewsTabHelper::FeedDetails> feed;

    if (contents) {
      auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
      subscribed = tab_helper->IsSubscribed();
      has_feeds = !tab_helper->GetAvailableFeeds().empty();
    }

    auto image = gfx::CreateVectorIcon(
        subscribed ? kBraveTodaySubscribedIcon : kBraveTodaySubscribeIcon, 16,
        color_utils::DeriveDefaultIconColor(GetIconColor(subscribed)));
    SetImage(ButtonState::STATE_NORMAL, image);
    SetVisible(has_feeds);
  }

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override {
    std::unique_ptr<views::LabelButtonBorder> border =
        LabelButton::CreateDefaultBorder();
    border->set_insets(gfx::Insets::VH(3, 0));
    return border;
  }

  std::u16string GetTooltipText(const gfx::Point& p) const override {
    return l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP);
  }

  SkPath GetHighlightPath() const {
    gfx::Rect rect(GetPreferredSize());
    const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
        views::Emphasis::kMaximum, rect.size());
    SkPath path;
    path.addRoundRect(gfx::RectToSkRect(rect), radii, radii);
    return path;
  }

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override {
    if (selection.active_tab_changed()) {
      if (selection.old_contents) {
        BraveNewsTabHelper::FromWebContents(selection.old_contents)
            ->RemoveObserver(this);
      }

      if (selection.new_contents) {
        BraveNewsTabHelper::FromWebContents(selection.new_contents)
            ->AddObserver(this);
      }
    }

    Update();
  }

  // BraveNewsTabHelper::PageFeedsObserver:
  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override {
    Update();
  }

  // views::View:
  void OnThemeChanged() override {
    views::LabelButton::OnThemeChanged();
    Update();
  }

 private:
  void ButtonPressed() {
    // If the bubble is already open, do nothing.
    if (bubble_widget_) {
      return;
    }

    bubble_widget_ =
        BraveNewsBubbleView::Show(this, tab_strip_->GetActiveWebContents());
  }

  SkColor GetIconColor(bool subscribed) const {
    if (!subscribed)
      return GetCurrentTextColor();

    return dark_mode::GetActiveBraveDarkModeType() ==
                   dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK
               ? kSubscribedDarkColor
               : kSubscribedLightColor;
  }

  BooleanPrefMember should_show_;
  BooleanPrefMember news_enabled_;

  base::raw_ptr<Profile> profile_;
  base::raw_ptr<TabStripModel> tab_strip_;
  base::WeakPtr<views::Widget> bubble_widget_;
};

BraveNewsLocationView::BraveNewsLocationView(Profile* profile,
                                             TabStripModel* tab_strip_model) {
  SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(3, 3)));

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetMainAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto* button = AddChildView(
      std::make_unique<BraveNewsButtonView>(profile, tab_strip_model));
  button->SetPreferredSize(gfx::Size(34, 24));
}

BraveNewsLocationView::~BraveNewsLocationView() = default;
