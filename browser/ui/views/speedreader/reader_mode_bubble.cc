/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_bubble.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/color/color_id.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace {

constexpr int kBubbleWidth = 256;

constexpr int kBoxLayoutChildSpacing = 16;
constexpr int kToggleLineHeight = 18;
constexpr int kToggleFontSize = 14;

constexpr int kNotesFontSize = 12;
constexpr int kNotesLineHeight = 16;

constexpr int kCornerRadius = 8;

}  // anonymous namespace

namespace speedreader {

ReaderModeBubble::ReaderModeBubble(views::View* anchor_view,
                                   SpeedreaderTabHelper* tab_helper)
    : LocationBarBubbleDelegateView(anchor_view, nullptr),
      tab_helper_(tab_helper) {
  DCHECK(GetSpeedreaderService());

  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  set_margins(gfx::Insets(0));
}

void ReaderModeBubble::Show() {
  ShowForReason(USER_GESTURE);
}

void ReaderModeBubble::Hide() {
  if (tab_helper_) {
    tab_helper_->OnBubbleClosed();
    tab_helper_ = nullptr;
  }
  CloseBubble();
}

gfx::Size ReaderModeBubble::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(
      kBubbleWidth,
      LocationBarBubbleDelegateView::CalculatePreferredSize(available_size)
          .height());
}

bool ReaderModeBubble::ShouldShowCloseButton() const {
  return false;
}

void ReaderModeBubble::WindowClosing() {
  if (tab_helper_) {
    tab_helper_->OnBubbleClosed();
    tab_helper_ = nullptr;
  }
}

void ReaderModeBubble::Init() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kBoxLayoutChildSpacing));

  SetPaintClientToLayer(true);
  set_use_round_corners(true);
  set_corner_radius(kCornerRadius);

  const auto add_toogle = [this](int ids, int acc_ids,
                                 const gfx::Insets& insets,
                                 const gfx::Insets& border) {
    auto font = gfx::FontList();
    font = font.DeriveWithSizeDelta(
        std::abs(kToggleFontSize - font.GetFontSize()));

    auto* box = AddChildView(std::make_unique<views::View>());

    auto* layout = box->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal, insets));

    if (!border.IsEmpty()) {
      box->SetBorder(
          views::CreateThemedSolidSidedBorder(border, ui::kColorMenuSeparator));
    }

    auto label = std::make_unique<views::Label>();
    label->SetText(brave_l10n::GetLocalizedResourceUTF16String(ids));
    label->SetFontList(font);
    label->SetLineHeight(kToggleLineHeight);
    label->SetMultiLine(true);
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    layout->SetFlexForView(box->AddChildView(std::move(label)),
                           1);  // show the button and force text to wrap
    layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kEnd);

    auto toggle = std::make_unique<views::ToggleButton>();
    toggle->SetAccessibleName(
        brave_l10n::GetLocalizedResourceUTF16String(acc_ids));
    return box->AddChildView(std::move(toggle));
  };

  // Always use speedreader for this site
  {
    site_toggle_ = add_toogle(IDS_READER_MODE_ALWAYS_LOAD_FOR_SITE_LABEL,
                              IDS_READER_MODE_ALWAYS_LOAD_FOR_SITE_ACC,
                              gfx::Insets::TLBR(24, 24, 0, 24), gfx::Insets());
    site_toggle_->SetCallback(base::BindRepeating(
        &ReaderModeBubble::OnSiteToggled, base::Unretained(this)));
    if (GetSpeedreaderService()->IsExplicitlyEnabledForSite(
            tab_helper_->web_contents())) {
      site_toggle_->SetIsOn(true);
    } else if (GetSpeedreaderService()->IsExplicitlyDisabledForSite(
                   tab_helper_->web_contents())) {
      site_toggle_->SetIsOn(false);
    } else {
      DistillState state = tab_helper_->PageDistillState();
      if (IsDistilledAutomatically(state)) {
        site_toggle_->SetIsOn(true);
      } else if (DistillStates::IsDistillable(state)) {
        site_toggle_->SetIsOn(false);
      }
    }
  }

  // Always use speedreader for all sites
  {
    all_sites_toggle_ = add_toogle(
        IDS_READER_MODE_ALWAYS_LOAD_FOR_ALL_SITES_LABEL,
        IDS_READER_MODE_ALWAYS_LOAD_FOR_ALL_SITES_ACC,
        gfx::Insets::TLBR(0, 24, 24, 24), gfx::Insets::TLBR(0, 0, 1, 0));
    all_sites_toggle_->SetCallback(base::BindRepeating(
        &ReaderModeBubble::OnAllSitesToggled, base::Unretained(this)));
    all_sites_toggle_->SetIsOn(GetSpeedreaderService()->IsEnabledForAllSites());
  }

  // Notes section
  {
    auto* box = AddChildView(std::make_unique<views::View>());
    auto* layout = box->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal,
        gfx::Insets::TLBR(0, 16, 16, 16)));
    layout->set_main_axis_alignment(
        views::BoxLayout::MainAxisAlignment::kCenter);
    layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);

    auto font = gfx::FontList();
    font =
        font.DeriveWithSizeDelta(std::abs(kNotesFontSize - font.GetFontSize()));

    auto label = std::make_unique<views::Label>();
    label->SetText(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_READER_MODE_NOTE_LABEL));
    label->SetFontList(font);
    label->SetMultiLine(true);
    label->SetLineHeight(kNotesLineHeight);
    label->SetEnabledColorId(ui::kColorSecondaryForeground);
    layout->SetFlexForView(box->AddChildView(std::move(label)), 1);
  }
}

SpeedreaderService* ReaderModeBubble::GetSpeedreaderService() {
  return speedreader::SpeedreaderServiceFactory::GetForBrowserContext(
      tab_helper_->web_contents()->GetBrowserContext());
}

void ReaderModeBubble::OnSiteToggled(const ui::Event& event) {
  DCHECK_EQ(event.target(), site_toggle_);
  const bool on = site_toggle_->GetIsOn();
  GetSpeedreaderService()->EnableForSite(tab_helper_->web_contents(), on);
}

void ReaderModeBubble::OnAllSitesToggled(const ui::Event& event) {
  DCHECK_EQ(event.target(), all_sites_toggle_);
  const bool on = all_sites_toggle_->GetIsOn();
  GetSpeedreaderService()->EnableForAllSites(on);
}

BEGIN_METADATA(ReaderModeBubble)
END_METADATA

}  // namespace speedreader
