/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <utility>

#include "base/check_deref.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/tabs/features.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/paint_vector_icon.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/containers/containers_menu_model.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/compositor/compositor.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"
#endif

using tabs::HorizontalTabsUpdateEnabled;

// static
gfx::Size BraveNewTabButton::GetButtonSize() {
  if (!HorizontalTabsUpdateEnabled()) {
    return {24, 24};
  }
  return {28, 28};
}

#if BUILDFLAG(ENABLE_CONTAINERS)
class BraveNewTabButton::NewTabButtonContainersMenuDelegate
    : public containers::ContainersMenuModel::Delegate {
 public:
  explicit NewTabButtonContainersMenuDelegate(
      BrowserWindowInterface& browser_window_interface)
      : browser_window_interface_(browser_window_interface) {}

  ~NewTabButtonContainersMenuDelegate() override = default;

  void OnContainerSelected(
      const containers::mojom::ContainerPtr& container) override {
    auto* browser = GetBrowserToOpenSettings();
    CHECK(browser);
    brave::OpenUrlInContainer(base::to_address(browser_window_interface_),
                              browser->GetNewTabURL(), container);
  }

  void OnNoContainerSelected() override {
    auto* browser = GetBrowserToOpenSettings();
    CHECK(browser);
    brave::OpenUrlWithoutContainer(base::to_address(browser_window_interface_),
                                   browser->GetNewTabURL());
  }

  // Unlike tab or link context menus, the new tab button is not tied to a
  // specific container, so no menu items should appear as "current".
  base::flat_set<std::string> GetCurrentContainerIds() override { return {}; }

  Browser* GetBrowserToOpenSettings() override {
    return browser_window_interface_->GetBrowserForMigrationOnly();
  }

  float GetScaleFactor() override {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(
        browser_window_interface_.operator->());
    CHECK(browser_view);
    auto* widget = browser_view->GetWidget();
    CHECK(widget);
    auto* compositor = widget->GetCompositor();
    CHECK(compositor);
    return compositor->device_scale_factor();
  }

 private:
  const raw_ref<BrowserWindowInterface> browser_window_interface_;
};
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

gfx::Size BraveNewTabButton::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Overridden so that we use Brave's custom button size
  gfx::Size size = GetButtonSize();
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

BraveNewTabButton::BraveNewTabButton(
    PressedCallback callback,
    const gfx::VectorIcon& icon,
    Edge fixed_flat_edge,
    Edge animated_flat_edge,
    BrowserWindowInterface* browser_window_interface)
    : NewTabButton(std::move(callback),
                   kLeoPlusAddIcon,
                   fixed_flat_edge,
                   animated_flat_edge,
                   browser_window_interface),
      browser_window_interface_(CHECK_DEREF(browser_window_interface)) {}

BraveNewTabButton::BraveNewTabButton(
    PressedCallback callback,
    const gfx::VectorIcon& icon /* this won't be used */,
    BrowserWindowInterface* browser_window_interface)
    : BraveNewTabButton(std::move(callback),
                        icon,
                        Edge::kNone,
                        Edge::kNone,
                        browser_window_interface) {}

BraveNewTabButton::~BraveNewTabButton() = default;

#if BUILDFLAG(ENABLE_CONTAINERS)
void BraveNewTabButton::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::mojom::MenuSourceType source_type) {
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    // Profile is guaranteed to be non-null because it's a
    // BrowserWindowInterface.
    auto* profile = browser_window_interface_->GetProfile();
    auto* service = ContainersServiceFactory::GetForProfile(profile);
    if (service && !(containers_context_menu_runner_ &&
                     containers_context_menu_runner_->IsRunning())) {
      // clear runner/model before the delegate so reopening the menu does not
      // destroy the delegate first.
      containers_context_menu_runner_.reset();
      containers_menu_model_.reset();
      containers_menu_delegate_.reset();

      containers_menu_delegate_ =
          std::make_unique<NewTabButtonContainersMenuDelegate>(
              *browser_window_interface_);
      containers_menu_model_ =
          std::make_unique<containers::ContainersMenuModel>(
              *containers_menu_delegate_, *service);
      containers_context_menu_runner_ = std::make_unique<views::MenuRunner>(
          containers_menu_model_.get(),
          views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU);
      // In some tests, we want to skip running the menu runner to avoid
      // blocking the test.
      if (!skip_containers_context_menu_runner_for_testing_) {
        containers_context_menu_runner_->RunMenuAt(
            source->GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
            views::MenuAnchorPosition::kTopLeft, source_type);
      }
      return;
    }
  }
  NewTabButton::ShowContextMenuForViewImpl(source, point, source_type);
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

BEGIN_METADATA(BraveNewTabButton)
END_METADATA
