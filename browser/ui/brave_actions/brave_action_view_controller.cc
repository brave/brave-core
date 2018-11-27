/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"

#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"

bool BraveActionViewController::IsEnabled(
    content::WebContents* web_contents) const {
  bool is_enabled = ExtensionActionViewController::IsEnabled(web_contents);
  if (is_enabled && extension_->id() == brave_rewards_extension_id &&
      browser_->profile()->IsOffTheRecord())
    is_enabled = false;
  return is_enabled;
}

void BraveActionViewController::HideActivePopup() {
  // Usually, for an extension this should call the main extensions
  // toolbar_actions_bar_->HideActivePopup(), but we don't have a reference
  // to that, and it doesn't seem neccessary, whether the extension is opened
  // via mouse or keyboard (if a `commands` extension property is present)
}

bool BraveActionViewController::DisabledClickOpensMenu() const {
  // disabled is a per-tab state
  return false;
}

ui::MenuModel* BraveActionViewController::GetContextMenu() {
  // no context menu for brave actions button
  return nullptr;
}

gfx::Image BraveActionViewController::GetIcon(
    content::WebContents* web_contents,
    const gfx::Size& size) {
  return gfx::Image(gfx::ImageSkia(GetIconImageSource(web_contents, size), size));
}

std::unique_ptr<BraveActionIconWithBadgeImageSource> BraveActionViewController::GetIconImageSource(
  content::WebContents* web_contents, const gfx::Size& size) {
  int tab_id = SessionTabHelper::IdForTab(web_contents).id();
  // generate icon
  std::unique_ptr<BraveActionIconWithBadgeImageSource> image_source(
      new BraveActionIconWithBadgeImageSource(size));
  image_source->SetIcon(icon_factory_.GetIcon(tab_id));
  // set text
  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  std::string badge_text = extension_action()->GetBadgeText(tab_id);
  if (!badge_text.empty()) {
    badge.reset(new IconWithBadgeImageSource::Badge(
            badge_text,
            extension_action()->GetBadgeTextColor(tab_id),
            extension_action()->GetBadgeBackgroundColor(tab_id)));
  }
  image_source->SetBadge(std::move(badge));
  // state
  // If the extension doesn't want to run on the active web contents, we
  // grayscale it to indicate that.
  image_source->set_grayscale(!IsEnabled(web_contents));
  image_source->set_paint_page_action_decoration(false);
  return image_source;
}