// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/toolbar/brave_app_menu_model.h"
#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"
#include "brave/browser/ui/views/bookmarks/brave_bookmark_bar_view.h"
#include "brave/browser/ui/views/bookmarks/floating_bookmark_bar_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/common/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/bookmarks/bookmark_stats.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/toolbar/bookmark_sub_menu_model.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view_test_helper.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_utils.h"
#include "url/gurl.h"

class FloatingBookmarkBarBrowserTest : public InProcessBrowserTest {
 protected:
  BraveBrowserView* view() {
    return BraveBrowserView::GetBrowserViewForBrowser(browser());
  }

  void SetMode(bookmarks::BookmarkBarVisibilityState mode) {
    browser()->GetProfile()->GetPrefs()->SetInteger(
        bookmarks::prefs::kBookmarkBarVisibilityState, static_cast<int>(mode));
    view()->DeprecatedLayoutImmediately();
  }

  void SetFloating(bool floating) {
    browser()->GetProfile()->GetPrefs()->SetBoolean(kFloatingBookmarkBar,
                                                    floating);
    view()->DeprecatedLayoutImmediately();
  }

  base::test::ScopedFeatureList feature_list_{
      features::kBraveFloatingBookmarkBar};
  gfx::ScopedAnimationDurationScaleMode no_animation_{
      gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION};
};

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       RevealPreservesPageBoundsAndNativeView) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetMode(bookmarks::BookmarkBarVisibilityState::kAlwaysHide);
  auto* native_bar = view()->GetBookmarkBarView();
  const gfx::Rect page_bounds = view()->multi_contents_view()->bounds();

  SetFloating(true);
  ASSERT_TRUE(view()->floating_bookmark_bar());
  EXPECT_EQ(native_bar, view()->GetBookmarkBarView());
  EXPECT_EQ(view(), native_bar->parent());
  EXPECT_FALSE(native_bar->GetVisible());
  EXPECT_EQ(page_bounds, view()->multi_contents_view()->bounds());
  const gfx::Size minimum_size = view()->GetMinimumSize();

  view()->OnFocusBookmarksToolbar();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  view()->DeprecatedLayoutImmediately();
  EXPECT_EQ(page_bounds, view()->multi_contents_view()->bounds());
  EXPECT_EQ(minimum_size, view()->GetMinimumSize());
  EXPECT_TRUE(
      native_bar->Contains(view()->GetFocusManager()->GetFocusedView()));

  SetFloating(false);
  SetMode(bookmarks::BookmarkBarVisibilityState::kAlwaysShow);
  EXPECT_FALSE(view()->floating_bookmark_bar());
  EXPECT_EQ(native_bar, view()->GetBookmarkBarView());
  EXPECT_EQ(view()->top_container(), native_bar->parent());
  EXPECT_TRUE(native_bar->GetVisible());
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       HoverRevealsAndLeavingHidesWithoutMovingPage) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetFloating(true);
  view()->GetActiveWebContents()->Focus();
  const gfx::Rect page_bounds = view()->multi_contents_view()->bounds();
  const gfx::Rect toolbar_bounds = view()->toolbar()->GetBoundsInScreen();
  ui::test::EventGenerator event_generator(
      views::GetRootWindow(view()->GetWidget()), view()->GetNativeWindow());
  for (const auto& point :
       {gfx::Point(toolbar_bounds.x(), toolbar_bounds.CenterPoint().y()),
        view()->GetLocationBarView()->GetBoundsInScreen().CenterPoint(),
        gfx::Point(toolbar_bounds.CenterPoint().x(),
                   toolbar_bounds.bottom())}) {
    event_generator.MoveMouseTo(point);
    EXPECT_FALSE(view()->GetBookmarkBarView()->GetVisible());
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
    EXPECT_EQ(page_bounds, view()->multi_contents_view()->bounds());

    event_generator.MoveMouseTo(
        view()->multi_contents_view()->GetBoundsInScreen().CenterPoint());
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
    EXPECT_EQ(page_bounds, view()->multi_contents_view()->bounds());
  }
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       HoverRevealsInInactiveWindow) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetFloating(true);
  chrome::FocusLocationBar(browser());
  auto* other_browser = CreateBrowser(browser()->GetProfile());
  auto* other_view = BraveBrowserView::GetBrowserViewForBrowser(other_browser);
  // An inactive window must still be exposed to receive hover input.
  gfx::Rect other_bounds = view()->multi_contents_view()->GetBoundsInScreen();
  other_bounds.SetVerticalBounds(
      view()->GetBookmarkBarView()->GetBoundsInScreen().bottom(),
      other_bounds.bottom());
  other_view->GetWidget()->SetBounds(other_bounds);
  other_view->GetWidget()->Activate();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !view()->GetWidget()->IsActive(); }));

  ui::test::EventGenerator event_generator(
      views::GetRootWindow(view()->GetWidget()), view()->GetNativeWindow());
  event_generator.MoveMouseTo(
      view()->GetLocationBarView()->GetBoundsInScreen().CenterPoint());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  EXPECT_FALSE(view()->GetWidget()->IsActive());

  event_generator.MoveMouseTo(
      view()->multi_contents_view()->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       FloatingBoundsCornersAndCenteredBookmarks) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  auto* model =
      BookmarkModelFactory::GetForBrowserContext(browser()->GetProfile());
  ASSERT_TRUE(base::test::RunUntil([&] { return model->loaded(); }));
  model->AddURL(model->bookmark_bar_node(), 0, u"Example",
                GURL("https://example.com"));
  SetFloating(true);
  view()->OnFocusBookmarksToolbar();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  view()->DeprecatedLayoutImmediately();
  auto* bar = view()->GetBookmarkBarView();
  const gfx::Rect bounds = bar->GetBoundsInScreen();
  const gfx::Rect contents = view()->multi_contents_view()->GetBoundsInScreen();
  constexpr int kExpectedMargin = 5;
  constexpr float kExpectedCornerRadius = 4.0f;
  EXPECT_EQ(gfx::RoundedCornersF(kExpectedCornerRadius),
            bar->layer()->rounded_corner_radii());
  EXPECT_EQ(contents.x() + kExpectedMargin, bounds.x());
  EXPECT_EQ(contents.right() - kExpectedMargin, bounds.right());
  EXPECT_EQ(view()->toolbar()->GetBoundsInScreen().bottom() + kExpectedMargin,
            bounds.y());

  BookmarkBarViewTestHelper helper(bar);
  ASSERT_EQ(1u, helper.GetBookmarkButtonCount());
  const gfx::Rect button = helper.GetBookmarkButton(0)->bounds();
  EXPECT_EQ(button.y(), bar->height() - button.bottom());
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       OutlineTracksContentThemeAndRestoresNativeMode) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetFloating(true);
  view()->OnFocusBookmarksToolbar();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  view()->DeprecatedLayoutImmediately();
  auto* bar = view()->GetBookmarkBarView();
  ASSERT_TRUE(bar->GetBorder());
  EXPECT_TRUE(bar->GetBorder()->GetInsets().IsEmpty());
  EXPECT_TRUE(bar->GetBorder()->color().IsLogical());

  auto* theme = ThemeServiceFactory::GetForProfile(browser()->GetProfile());
  for (auto scheme : {ThemeService::BrowserColorScheme::kLight,
                      ThemeService::BrowserColorScheme::kDark}) {
    theme->SetBrowserColorScheme(scheme);
    gfx::Canvas actual(bar->size(), 1.0f, /*is_opaque=*/false);
    bar->GetBorder()->Paint(*bar, &actual);
    const SkColor painted = actual.GetBitmap().getColor(bar->width() / 2, 0);
    const SkColor expected =
        bar->GetColorProvider()->GetColor(kColorBraveContentsOutline);
    // Edge coverage changes alpha, and unpremultiplication can round RGB by
    // one.
    constexpr int kChannelRoundingTolerance = 1;
    EXPECT_GT(SkColorGetA(painted), 0u);
    EXPECT_NEAR(SkColorGetR(expected), SkColorGetR(painted),
                kChannelRoundingTolerance);
    EXPECT_NEAR(SkColorGetG(expected), SkColorGetG(painted),
                kChannelRoundingTolerance);
    EXPECT_NEAR(SkColorGetB(expected), SkColorGetB(painted),
                kChannelRoundingTolerance);
  }

  SetFloating(false);
  SetMode(bookmarks::BookmarkBarVisibilityState::kAlwaysShow);
  EXPECT_FALSE(bar->GetBorder());
}

IN_PROC_BROWSER_TEST_F(
    FloatingBookmarkBarBrowserTest,
    NewTabShowsAutomaticallyAndModeTransitionsRestoreParent) {
  chrome::NewTab(browser(), NewTabTypes::kNewTabCommand);
  SetFloating(true);
  auto* native_bar = view()->GetBookmarkBarView();
  ASSERT_TRUE(view()->floating_bookmark_bar());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  EXPECT_TRUE(native_bar->GetVisible());

  SetFloating(false);
  SetMode(bookmarks::BookmarkBarVisibilityState::kAlwaysHide);
  EXPECT_FALSE(view()->floating_bookmark_bar());
  EXPECT_EQ(view()->top_container(), native_bar->parent());
  EXPECT_TRUE(
      base::test::RunUntil([&] { return !view()->IsBookmarkBarVisible(); }));

  SetMode(bookmarks::BookmarkBarVisibilityState::kOnlyShowOnNtp);
  EXPECT_FALSE(view()->floating_bookmark_bar());
  EXPECT_EQ(native_bar, view()->GetBookmarkBarView());
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       NewTabsRevealAndTypingSuppressesWithoutMovingPage) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetFloating(true);
  const gfx::Rect page_bounds = view()->multi_contents_view()->bounds();
  auto* original_contents = view()->GetActiveWebContents();
  // Park the pointer in the page before the New Tab opens; moving into the
  // page afterwards would end the automatic reveal.
  ui::test::EventGenerator event_generator(
      views::GetRootWindow(view()->GetWidget()), view()->GetNativeWindow());
  event_generator.MoveMouseTo(
      view()->multi_contents_view()->GetBoundsInScreen().CenterPoint());
  chrome::NewTab(browser(), NewTabTypes::kNewTabCommand);
  ASSERT_TRUE(base::test::RunUntil([&] {
    return view()->GetActiveWebContents() != original_contents &&
           view()->floating_bookmark_bar()->IsRevealed();
  }));
  EXPECT_EQ(page_bounds, view()->multi_contents_view()->bounds());

  chrome::FocusLocationBar(browser());
  event_generator.PressKey(ui::VKEY_A, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_A, ui::EF_NONE);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
  event_generator.PressKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  auto* edit_model =
      view()->GetLocationBarView()->GetOmniboxController()->edit_model();
  // Native Escape closes suggestions before a subsequent Escape reverts input.
  if (edit_model->user_input_in_progress()) {
    event_generator.PressKey(ui::VKEY_ESCAPE, ui::EF_NONE);
    event_generator.ReleaseKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  }
  ASSERT_FALSE(edit_model->user_input_in_progress());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));

  event_generator.PressKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
  view()->DeprecatedLayoutImmediately();
  EXPECT_FALSE(view()->GetBookmarkBarView()->GetVisible());

  browser()->GetTabStripModel()->ActivateTabAt(0);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->GetActiveWebContents() == original_contents; }));
  EXPECT_FALSE(view()->floating_bookmark_bar()->IsRevealed());
  browser()->GetTabStripModel()->ActivateTabAt(1);
  view()->GetActiveWebContents()->Focus();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  EXPECT_EQ(page_bounds, view()->multi_contents_view()->bounds());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       NewTabRevealEndsWhenPointerEntersPage) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetFloating(true);
  const gfx::Point page_center =
      view()->multi_contents_view()->GetBoundsInScreen().CenterPoint();
  ui::test::EventGenerator event_generator(
      views::GetRootWindow(view()->GetWidget()), view()->GetNativeWindow());
  event_generator.MoveMouseTo(page_center);
  chrome::NewTab(browser(), NewTabTypes::kNewTabCommand);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));

  event_generator.MoveMouseTo(page_center + gfx::Vector2d(0, 10));
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));

  // Toolbar hover still reveals the bar on the same New Tab.
  event_generator.MoveMouseTo(
      view()->GetLocationBarView()->GetBoundsInScreen().CenterPoint());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  event_generator.MoveMouseTo(page_center);
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       KeyboardCanRevealFromOmniboxAndEditingDismisses) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  SetFloating(true);
  chrome::FocusLocationBar(browser());
  view()->OnFocusBookmarksToolbar();
  EXPECT_TRUE(view()->GetBookmarkBarView()->GetVisible());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return view()->floating_bookmark_bar()->IsRevealed(); }));
  chrome::FocusLocationBar(browser());
  ui::test::EventGenerator event_generator(
      views::GetRootWindow(view()->GetWidget()), view()->GetNativeWindow());
  event_generator.PressKey(ui::VKEY_A, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_A, ui::EF_NONE);
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return !view()->floating_bookmark_bar()->IsRevealed(); }));
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       MenusShareSelectionWithoutChangingSyncedEnum) {
  auto* prefs = browser()->GetProfile()->GetPrefs();
  SetMode(bookmarks::BookmarkBarVisibilityState::kAlwaysHide);
  BraveAppMenuModel app_menu(view()->toolbar(), browser());
  app_menu.Init();
  auto* root = app_menu.bookmark_sub_menu_model();
  ASSERT_TRUE(root);
  const auto visibility_index =
      root->GetIndexOfCommandId(IDC_BOOKMARK_BAR_SUBMENU);
  ASSERT_TRUE(visibility_index);
  auto* submenu = root->GetSubmenuModelAt(*visibility_index);
  std::optional<size_t> floating_index;
  for (size_t i = 0; i < submenu->GetItemCount(); ++i) {
    if (submenu->GetCommandIdAt(i) ==
        IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING) {
      floating_index = i;
    }
  }
  ASSERT_TRUE(floating_index);
  EXPECT_EQ(u"Auto hiding (floating)", submenu->GetLabelAt(*floating_index));
  submenu->ActivatedAt(*floating_index);
  EXPECT_TRUE(prefs->GetBoolean(kFloatingBookmarkBar));
  EXPECT_EQ(
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysHide),
      prefs->GetInteger(bookmarks::prefs::kBookmarkBarVisibilityState));

  auto* model =
      BookmarkModelFactory::GetForBrowserContext(browser()->GetProfile());
  BraveBookmarkContextMenuController context_menu(
      view()->GetNativeWindow(), nullptr, browser(), browser()->GetProfile(),
      BookmarkLaunchLocation::kAttachedBar, {model->bookmark_bar_node()},
      false);
  EXPECT_TRUE(context_menu.IsCommandIdChecked(
      IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING));
  EXPECT_TRUE(
      app_menu.IsCommandIdChecked(IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING));
  for (int command : {IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_SHOW,
                      IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_HIDE,
                      IDC_BOOKMARK_BAR_SUBMENU_ONLY_ON_NTP}) {
    EXPECT_FALSE(app_menu.IsCommandIdChecked(command));
    EXPECT_FALSE(context_menu.IsCommandIdChecked(command));
    context_menu.ExecuteCommand(command, 0);
    EXPECT_FALSE(prefs->GetBoolean(kFloatingBookmarkBar));
    EXPECT_TRUE(context_menu.IsCommandIdChecked(command));
    EXPECT_TRUE(app_menu.IsCommandIdChecked(command));
    context_menu.ExecuteCommand(IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING, 0);
  }
  auto* delegate = static_cast<ui::SimpleMenuModel::Delegate*>(&app_menu);
  delegate->ExecuteCommand(IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_HIDE, 0);
  EXPECT_FALSE(prefs->GetBoolean(kFloatingBookmarkBar));
  EXPECT_TRUE(
      context_menu.IsCommandIdChecked(IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_HIDE));
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       SettingsDropdownKeepsFloatingLocal) {
  SetMode(bookmarks::BookmarkBarVisibilityState::kAlwaysHide);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/appearance")));
  auto* contents = view()->GetActiveWebContents();
  ASSERT_EQ(true, content::EvalJs(contents, R"JS(
    (async () => {
      await customElements.whenDefined('settings-appearance-page');
      const page = await new Promise(resolve => {
        const observer = new MutationObserver(scan);
        const observed = new WeakSet();
        function find(root) {
          if (!observed.has(root)) {
            observed.add(root);
            observer.observe(root, {childList: true, subtree: true});
          }
          for (const element of root.querySelectorAll('*')) {
            if (element.localName === 'settings-appearance-page') {
              return element;
            }
            if (element.shadowRoot) {
              const match = find(element.shadowRoot);
              if (match) return match;
            }
          }
          return null;
        }
        function scan() {
          const page = find(document);
          if (page) { observer.disconnect(); resolve(page); }
        }
        scan();
      });
      await page.updateComplete;
      await page.updateComplete;
      window.floatingTestPage = page;
      const dropdown =
          page.shadowRoot.querySelector('#bookmarksBarVisibilityDropdown');
      await dropdown.updateComplete;
      if (!dropdown.menuOptions.some(option => option.value === 'floating' &&
          option.name === 'Auto hiding (floating)')) return false;
      dropdown.$.dropdownMenu.value = 'floating';
      dropdown.$.dropdownMenu.dispatchEvent(
          new Event('change', {bubbles: true}));
      await page.updateComplete;
      await dropdown.updateComplete;
      return dropdown.getSelectedValue() === 'floating';
    })()
  )JS"));
  auto* prefs = browser()->GetProfile()->GetPrefs();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return prefs->GetBoolean(kFloatingBookmarkBar); }));
  EXPECT_EQ(
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysHide),
      prefs->GetInteger(bookmarks::prefs::kBookmarkBarVisibilityState));
  ASSERT_EQ(true, content::EvalJs(contents, R"JS(
    (async () => {
      const page = window.floatingTestPage;
      const dropdown =
          page.shadowRoot.querySelector('#bookmarksBarVisibilityDropdown');
      dropdown.$.dropdownMenu.value = '1';
      dropdown.$.dropdownMenu.dispatchEvent(
          new Event('change', {bubbles: true}));
      await page.updateComplete;
      await dropdown.updateComplete;
      return dropdown.getSelectedValue() === '1';
    })()
  )JS"));
  EXPECT_TRUE(base::test::RunUntil([&] {
    return !prefs->GetBoolean(kFloatingBookmarkBar) &&
           prefs->GetInteger(bookmarks::prefs::kBookmarkBarVisibilityState) ==
               static_cast<int>(
                   bookmarks::BookmarkBarVisibilityState::kOnlyShowOnNtp);
  }));
}

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarBrowserTest,
                       BoundsExcludeEitherVerticalTabEdge) {
  const gfx::Rect page(100, 80, 1000, 600);
  const gfx::Rect toolbar(100, 40, 1000, 40);
  constexpr int kMargin = 8;
  constexpr int kBarHeight = 32;

  EXPECT_EQ(gfx::Rect(108, 88, 984, kBarHeight),
            FloatingBookmarkBarController::CalculateBounds(
                page, toolbar, std::nullopt, kMargin, kBarHeight));
  EXPECT_EQ(
      gfx::Rect(308, 88, 784, kBarHeight),
      FloatingBookmarkBarController::CalculateBounds(
          page, toolbar, gfx::Rect(100, 80, 200, 600), kMargin, kBarHeight));
  EXPECT_EQ(
      gfx::Rect(108, 88, 784, kBarHeight),
      FloatingBookmarkBarController::CalculateBounds(
          page, toolbar, gfx::Rect(900, 80, 200, 600), kMargin, kBarHeight));
  EXPECT_TRUE(FloatingBookmarkBarController::CalculateBounds(
                  gfx::Rect(100, 80, 10, 10), toolbar, std::nullopt, kMargin,
                  kBarHeight)
                  .IsEmpty());
}

class FloatingBookmarkBarDisabledBrowserTest : public InProcessBrowserTest {
 public:
  FloatingBookmarkBarDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveFloatingBookmarkBar);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(FloatingBookmarkBarDisabledBrowserTest,
                       PrefIsIgnoredWithoutFeature) {
  auto* view = BraveBrowserView::GetBrowserViewForBrowser(browser());
  browser()->GetProfile()->GetPrefs()->SetBoolean(kFloatingBookmarkBar, true);
  view->DeprecatedLayoutImmediately();
  EXPECT_FALSE(view->floating_bookmark_bar());
  EXPECT_EQ(view->top_container(), view->GetBookmarkBarView()->parent());
}
