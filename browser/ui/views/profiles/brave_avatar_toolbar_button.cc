#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"

gfx::ImageSkia BraveAvatarToolbarButton::GetAvatarIcon() const {
  if (profile_->IsTorProfile()) {
    const int icon_size =
      ui::MaterialDesignController::IsTouchOptimizedUiEnabled() ? 24 : 20;
    const SkColor icon_color =
      GetThemeProvider()->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
    return gfx::CreateVectorIcon(kTorProfileIcon, icon_size, icon_color);
  }
  return AvatarToolbarButton::GetAvatarIcon();
}