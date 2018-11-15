/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_theme_event_router.h"

#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/extensions/api/brave_theme.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_event_histogram_value.h"

using BTS = BraveThemeService;

namespace extensions {

class BraveThemeEventRouterImpl : public BraveThemeEventRouter {
 public:
  BraveThemeEventRouterImpl() {}
  ~BraveThemeEventRouterImpl() override {}

  void OnBraveThemeTypeChanged(Profile* profile) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveThemeEventRouterImpl);
};

void BraveThemeEventRouterImpl::OnBraveThemeTypeChanged(Profile* profile) {
  EventRouter* event_router = EventRouter::Get(profile);
  const std::string theme_type = BTS::GetStringFromBraveThemeType(
      BTS::GetActiveBraveThemeType(profile));

  auto event = std::make_unique<extensions::Event>(
      extensions::events::BRAVE_ON_BRAVE_THEME_TYPE_CHANGED,
      api::brave_theme::OnBraveThemeTypeChanged::kEventName,
      api::brave_theme::OnBraveThemeTypeChanged::Create(theme_type),
      profile);

  event_router->BroadcastEvent(std::move(event));
}

// static
std::unique_ptr<BraveThemeEventRouter> BraveThemeEventRouter::Create() {
  return std::make_unique<BraveThemeEventRouterImpl>();
}

}  // namespace extensions
