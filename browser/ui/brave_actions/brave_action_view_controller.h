// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_CONTROLLER_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/extensions/extension_action_view_controller.h"
#include "extensions/common/extension_id.h"

class BraveActionIconWithBadgeImageSource;
class Browser;
class ExtensionAction;
class ExtensionRegistry;
class ExtensionsContainer;
class ToolbarActionViewController;

namespace ui {
class MenuModel;
}

// The purposes of this subclass are to:
// - Overcome the DCHECK in HideActivePopup since a toolbar will not be provided
// - Use our custom class for painting the badge differently compared to
//   user-installed extensions
// - Remove the context menu from the button since we do not allow uninstall
class BraveActionViewController : public ExtensionActionViewController {
 public:
  BraveActionViewController(const BraveActionViewController&) = delete;
  BraveActionViewController& operator=(const BraveActionViewController&) =
      delete;

  static std::unique_ptr<BraveActionViewController> Create(
      const extensions::ExtensionId& extension_id,
      Browser* browser,
      ExtensionsContainer* extensions_container);

  bool IsEnabled(content::WebContents* web_contents) const override;
  gfx::Image GetIcon(content::WebContents* web_contents,
                     const gfx::Size& size) override;
  ui::MenuModel* GetContextMenu() override;
  bool ExecuteActionUI(std::string relative_path);
  ToolbarActionViewController* GetExtensionViewController(
      const std::string& extension_id);

 private:
  // New instances should be instantiated with Create().
  BraveActionViewController(
      scoped_refptr<const extensions::Extension> extension,
      Browser* browser,
      extensions::ExtensionAction* extension_action,
      extensions::ExtensionRegistry* extension_registry,
      ExtensionsContainer* extensions_container);

  ExtensionActionViewController* GetPreferredPopupViewController() override;
  bool TriggerPopupWithUrl(PopupShowAction show_action,
                           const GURL& popup_url,
                           bool grant_tab_permissions) override;
  void OnPopupClosed() override;
  // Returns the image source for the icon.
  std::unique_ptr<BraveActionIconWithBadgeImageSource> GetIconImageSource(
      content::WebContents* web_contents,
      const gfx::Size& size);
};

#endif  // BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_CONTROLLER_H_
