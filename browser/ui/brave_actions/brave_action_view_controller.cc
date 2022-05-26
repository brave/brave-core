/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/extensions/extension_view_host.h"
#include "chrome/browser/extensions/extension_view_host_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/extensions/extension_popup_types.h"
#include "chrome/browser/ui/extensions/extensions_container.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_delegate.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_action.h"
#include "extensions/browser/extension_action_manager.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/api/extension_action/action_info.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/permissions/api_permission.h"
#include "ui/base/theme_provider.h"
#include "ui/color/color_provider_manager.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/native_theme/native_theme.h"

BraveActionViewController::BraveActionViewController(
    scoped_refptr<const extensions::Extension> extension,
    Browser* browser,
    extensions::ExtensionAction* extension_action,
    extensions::ExtensionRegistry* extension_registry,
    ExtensionsContainer* extensions_container)
    : ExtensionActionViewController(std::move(extension),
                                    browser,
                                    extension_action,
                                    extension_registry,
                                    extensions_container) {}

bool BraveActionViewController::IsEnabled(
    content::WebContents* web_contents) const {
  bool is_enabled = ExtensionActionViewController::IsEnabled(web_contents);
  if (is_enabled && extension_->id() == brave_rewards_extension_id &&
      !brave::IsRegularProfile(browser_->profile()))
    is_enabled = false;
  return is_enabled;
}

ui::MenuModel* BraveActionViewController::GetContextMenu(
    extensions::ExtensionContextMenuModel::ContextMenuSource
        context_menu_source) {
  // no context menu for brave actions button
  return nullptr;
}

void BraveActionViewController::ExecuteActionUI(
    const std::string& relative_path) {
  TriggerPopupWithUrl(PopupShowAction::kShow,
                      extension()->GetResourceURL(relative_path),
                      /*grant_tab_permissions=*/true, ShowPopupCallback());
}

ExtensionActionViewController*
BraveActionViewController::GetPreferredPopupViewController() {
  return this;
}

void BraveActionViewController::TriggerPopup(PopupShowAction show_action,
                                             bool grant_tab_permissions,
                                             ShowPopupCallback callback) {
  content::WebContents* const web_contents =
      view_delegate_->GetCurrentWebContents();
  if (!web_contents)
    return;

  const int tab_id = sessions::SessionTabHelper::IdForTab(web_contents).id();
  TriggerPopupWithUrl(show_action, extension_action_->GetPopupUrl(tab_id),
                      grant_tab_permissions, std::move(callback));
}

void BraveActionViewController::OnPopupClosed() {
  DCHECK(popup_host_observation_.IsObservingSource(popup_host_));
  popup_host_observation_.Reset();
  popup_host_ = nullptr;
  extensions_container_->SetPopupOwner(nullptr);
  view_delegate_->OnPopupClosed();
}

gfx::Image BraveActionViewController::GetIcon(
    content::WebContents* web_contents,
    const gfx::Size& size) {
  return gfx::Image(
      gfx::ImageSkia(GetIconImageSource(web_contents, size), size));
}

void BraveActionViewController::TriggerPopupWithUrl(
    PopupShowAction show_action,
    const GURL& popup_url,
    bool grant_tab_permissions,
    ShowPopupCallback callback) {
  // If this extension is currently showing a popup, hide it. This behavior is
  // a bit different than ExtensionActionViewController, which will hide any
  // popup, regardless of extension. Consider duplicating the original behavior.
  HidePopup();

  std::unique_ptr<extensions::ExtensionViewHost> host =
      extensions::ExtensionViewHostFactory::CreatePopupHost(popup_url,
                                                            browser_);
  if (!host)
    return;

  popup_host_ = host.get();
  popup_host_observation_.Observe(popup_host_);
  extensions_container_->SetPopupOwner(this);
  ShowPopup(std::move(host), grant_tab_permissions, show_action,
            std::move(callback));
}

std::unique_ptr<BraveActionIconWithBadgeImageSource>
BraveActionViewController::GetIconImageSource(
  content::WebContents* web_contents, const gfx::Size& size) {
  int tab_id = sessions::SessionTabHelper::IdForTab(web_contents).id();
  // generate icon
  // `web_contents` may be null during tab closure or in tests.  Fall back on a
  // generic color provider.
  auto get_color_provider_callback = base::BindRepeating(
      [](base::WeakPtr<content::WebContents> weak_web_contents) {
        const auto* const color_provider =
            weak_web_contents
                ? &weak_web_contents->GetColorProvider()
                : ui::ColorProviderManager::Get().GetColorProviderFor(
                      ui::NativeTheme::GetInstanceForNativeUi()
                          ->GetColorProviderKey(nullptr));
        return color_provider;
      },
      web_contents ? web_contents->GetWeakPtr()
                   : base::WeakPtr<content::WebContents>());

  std::unique_ptr<BraveActionIconWithBadgeImageSource> image_source(
      new BraveActionIconWithBadgeImageSource(
          size, std::move(get_color_provider_callback)));
  image_source->SetIcon(icon_factory_.GetIcon(tab_id));
  // set text
  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  std::string badge_text =
      extension_action()->GetExplicitlySetBadgeText(tab_id);
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
  return image_source;
}
