/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/feature_list.h"
#include "brave/common/url_constants.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/features.h"
#include "components/permissions/permission_prompt.h"
#include "components/permissions/permission_request.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/models/combobox_model.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/window/dialog_delegate.h"

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/permissions/request_type.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/style/typography.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_WIDEVINE)
class DontAskAgainCheckbox : public views::Checkbox {
 public:
  explicit DontAskAgainCheckbox(WidevinePermissionRequest* request);
  DontAskAgainCheckbox(const DontAskAgainCheckbox&) = delete;
  DontAskAgainCheckbox& operator=(const DontAskAgainCheckbox&) = delete;

 private:
  void ButtonPressed();

  WidevinePermissionRequest* request_;
};

DontAskAgainCheckbox::DontAskAgainCheckbox(WidevinePermissionRequest* request)
    : views::Checkbox(
          l10n_util::GetStringUTF16(IDS_WIDEVINE_DONT_ASK_AGAIN_CHECKBOX),
          base::BindRepeating(&DontAskAgainCheckbox::ButtonPressed,
                              base::Unretained(this))),
      request_(request) {}

void DontAskAgainCheckbox::ButtonPressed() {
  request_->set_dont_ask_widevine_install(GetChecked());
}

bool HasWidevinePermissionRequest(
    const std::vector<permissions::PermissionRequest*>& requests) {
  // When widevine permission is requested, |requests| only includes Widevine
  // permission because it is not a candidate for grouping.
  if (requests.size() == 1 &&
      requests[0]->GetRequestType() == permissions::RequestType::kWidevine)
    return true;

  return false;
}

void AddAdditionalWidevineViewControlsIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<permissions::PermissionRequest*>& requests) {
  if (!HasWidevinePermissionRequest(requests))
    return;

  auto* widevine_request = static_cast<WidevinePermissionRequest*>(requests[0]);
  views::Label* text = new views::Label(
      widevine_request->GetExplanatoryMessageText(),
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY);
  text->SetMultiLine(true);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const int preferred_dialog_width = provider->GetSnappedDialogWidth(
      dialog_delegate_view->GetPreferredSize().width());
  // Resize width. Then, it's height deduced.
  text->SizeToFit(preferred_dialog_width -
                  dialog_delegate_view->margins().width());
  dialog_delegate_view->AddChildView(text);
  dialog_delegate_view->AddChildView(
      new DontAskAgainCheckbox(widevine_request));
}
#else
void AddAdditionalWidevineViewControlsIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<permissions::PermissionRequest*>& requests) {}
#endif

// Custom combobox, shows permission lifetime options and applies selected value
// to all permissions currently visible in the bubble.
class PermissionLifetimeCombobox : public views::Combobox,
                                   public ui::ComboboxModel {
 public:
  explicit PermissionLifetimeCombobox(
      permissions::PermissionPrompt::Delegate* delegate)
      : delegate_(delegate),
        lifetime_options_(permissions::CreatePermissionLifetimeOptions()) {
    DCHECK(delegate_);
    SetCallback(base::BindRepeating(&PermissionLifetimeCombobox::OnItemSelected,
                                    base::Unretained(this)));
    SetModel(this);
    OnItemSelected();
  }

  PermissionLifetimeCombobox(const PermissionLifetimeCombobox&) = delete;
  PermissionLifetimeCombobox& operator=(const PermissionLifetimeCombobox&) =
      delete;

  // ui::ComboboxModel:
  int GetItemCount() const override { return lifetime_options_.size(); }

  std::u16string GetItemAt(int index) const override {
    return lifetime_options_[index].label;
  }

 private:
  void OnItemSelected() {
    SetRequestsLifetime(lifetime_options_, GetSelectedIndex(), delegate_);
  }

  permissions::PermissionPrompt::Delegate* const delegate_;
  std::vector<permissions::PermissionLifetimeOption> lifetime_options_;
};

views::View* AddPermissionLifetimeComboboxIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    permissions::PermissionPrompt::Delegate* delegate) {
  if (!ShouldShowLifetimeOptions(delegate)) {
    return nullptr;
  }

  // Create a single line container for a label and a combobox.
  auto container = std::make_unique<views::View>();
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      views::LayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_RELATED_BUTTON_HORIZONTAL)));

  // Add the label.
  auto label = std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_LIFETIME_COMBOBOX_LABEL),
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY);
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  container->AddChildView(std::move(label));

  // Add the combobox.
  auto* combobox = container->AddChildView(
      std::make_unique<PermissionLifetimeCombobox>(delegate));
  static_cast<views::BoxLayout*>(container->GetLayoutManager())
      ->SetFlexForView(combobox, 1);

  // Add the container to the view.
  return dialog_delegate_view->AddChildView(std::move(container));
}

void AddFootnoteViewIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    Browser* browser) {
  if (!base::FeatureList::IsEnabled(
          permissions::features::kPermissionLifetime)) {
    return;
  }

  std::vector<std::u16string> replacements{
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_SITE_PERMISSION_LINK),
      l10n_util::GetStringUTF16(IDS_LEARN_MORE)};
  std::vector<size_t> offsets;
  std::u16string footnote_text = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF16(IDS_PERMISSIONS_BUBBLE_FOOTNOTE_TEXT),
      replacements, &offsets);

  auto label = std::make_unique<views::StyledLabel>();
  label->SetText(footnote_text);
  label->SetDefaultTextStyle(views::style::STYLE_SECONDARY);

  auto add_link = [&](size_t idx, GURL url) {
    DCHECK(idx < offsets.size());
    DCHECK(idx < replacements.size());

    gfx::Range link_range(offsets[idx],
                          offsets[idx] + replacements[idx].length());

    views::StyledLabel::RangeStyleInfo link_style =
        views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
            [](Browser* browser, const GURL& url) {
              chrome::AddSelectedTabWithURL(browser, url,
                                            ui::PAGE_TRANSITION_LINK);
            },
            base::Unretained(browser), std::move(url)));

    label->AddStyleRange(link_range, link_style);
  };

  add_link(0, GURL(chrome::kChromeUIContentSettingsURL));
  add_link(1, GURL(kPermissionPromptLearnMoreUrl));

  dialog_delegate_view->SetFootnoteView(std::move(label));
}

}  // namespace

#define BRAVE_PERMISSION_PROMPT_BUBBLE_VIEW                               \
  AddAdditionalWidevineViewControlsIfNeeded(this, delegate_->Requests()); \
  auto* permission_lifetime_view =                                        \
      AddPermissionLifetimeComboboxIfNeeded(this, delegate_);             \
  AddFootnoteViewIfNeeded(this, browser_);                                \
  if (permission_lifetime_view) {                                         \
    set_fixed_width(                                                      \
        std::max(GetPreferredSize().width(),                              \
                 permission_lifetime_view->GetPreferredSize().width()) +  \
        margins().width());                                               \
    set_should_ignore_snapping(true);                                     \
  }

#include "../../../../../../../chrome/browser/ui/views/permission_bubble/permission_prompt_bubble_view.cc"
#undef BRAVE_PERMISSION_PROMPT_BUBBLE_VIEW
