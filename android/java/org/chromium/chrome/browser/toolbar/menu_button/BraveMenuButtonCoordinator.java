/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.menu_button;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.widget.ImageButton;

import androidx.annotation.IdRes;
import androidx.core.content.ContextCompat;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

import java.util.function.Supplier;

public class BraveMenuButtonCoordinator extends MenuButtonCoordinator {
    private final Activity mActivity;
    private boolean mIsCustomIconApplied;

    public BraveMenuButtonCoordinator(
            Activity activity,
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            BrowserStateBrowserControlsVisibilityDelegate controlsVisibilityDelegate,
            WindowAndroid windowAndroid,
            SetFocusFunction setUrlBarFocusFunction,
            Runnable requestRenderRunnable,
            boolean canShowAppUpdateBadge,
            Supplier<Boolean> isInOverviewModeSupplier,
            ThemeColorProvider themeColorProvider,
            IncognitoStateProvider incognitoStateProvider,
            Supplier<@Nullable MenuButtonState> menuButtonStateSupplier,
            Runnable onMenuButtonClicked,
            @IdRes int menuButtonId,
            @Nullable VisibilityDelegate visibilityDelegate,
            boolean isWebApp) {
        super(
                activity,
                appMenuCoordinatorSupplier,
                controlsVisibilityDelegate,
                windowAndroid,
                setUrlBarFocusFunction,
                requestRenderRunnable,
                canShowAppUpdateBadge,
                isInOverviewModeSupplier,
                themeColorProvider,
                incognitoStateProvider,
                menuButtonStateSupplier,
                onMenuButtonClicked,
                menuButtonId,
                visibilityDelegate,
                isWebApp);

        mActivity = windowAndroid.getActivity().get();

        // Set the custom Brave menu icon (logo + three dots)
        // Only do this when the menu is shown in the top toolbar
        // (i.e., when bottom toolbar is disabled)
        customizeMenuButton();
    }

    @Override
    public void onTintChanged(
            android.content.res.ColorStateList tint,
            android.content.res.ColorStateList activityFocusTint,
            int brandedColorScheme) {
        // Call super first to update other components
        super.onTintChanged(tint, activityFocusTint, brandedColorScheme);

        // Re-apply our custom icon after tint changes if we've customized it
        if (mIsCustomIconApplied) {
            customizeMenuButton();
        }
    }

    private void customizeMenuButton() {
        // Get the menu button and set the custom drawable
        MenuButton menuButton = super.getMenuButton();
        if (menuButton != null) {
            ImageButton imageButton = menuButton.findViewById(R.id.menu_button);
            if (imageButton != null) {
                // Create a combined bitmap drawable instead of using LayerDrawable
                // to avoid ClassCastException (MenuButton expects BitmapDrawable)
                BitmapDrawable combinedDrawable = createCombinedMenuIcon();
                if (combinedDrawable != null) {
                    // Clear any tint from the ImageButton to preserve original colors
                    imageButton.setImageTintList(null);
                    imageButton.setImageDrawable(combinedDrawable);

                    // Set custom rounded rectangle background that fits the combined icon
                    imageButton.setBackgroundResource(R.drawable.brave_menu_button_background);

                    // Set the ImageButton to wrap content so it fits the icon size
                    android.view.ViewGroup.LayoutParams params = imageButton.getLayoutParams();
                    if (params != null) {
                        params.width = android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
                        params.height = android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
                        imageButton.setLayoutParams(params);
                    }

                    // Add padding to ensure the icon fits well within the rounded rectangle
                    int paddingPx =
                            (int) (4 * mActivity.getResources().getDisplayMetrics().density);
                    imageButton.setPadding(paddingPx, paddingPx, paddingPx, paddingPx);

                    // Ensure it's centered
                    imageButton.setScaleType(android.widget.ImageView.ScaleType.FIT_CENTER);

                    mIsCustomIconApplied = true;
                }
            }
        }
    }

    private BitmapDrawable createCombinedMenuIcon() {
        try {
            // Load the Brave logo and three dots drawables
            Drawable braveLogoDrawable =
                    ContextCompat.getDrawable(mActivity, R.drawable.ic_brave_logo);
            Drawable threeDotsDrawable =
                    ContextCompat.getDrawable(mActivity, R.drawable.ic_more_vert_24dp);

            if (braveLogoDrawable == null || threeDotsDrawable == null) {
                return null;
            }

            // Mutate the drawables so we don't affect other instances
            braveLogoDrawable = braveLogoDrawable.mutate();
            threeDotsDrawable = threeDotsDrawable.mutate();

            // Clear any tint from the Brave logo to show its original gradient colors
            braveLogoDrawable.setTintList(null);

            // Define sizes - negative spacing to overlap slightly like in Figma
            int logoSize = 18; // 18dp for the logo
            int dotsSize = 24; // 24dp for the three dots
            int spacing = -4; // -4dp spacing (overlap) to match Figma
            int horizontalMargin = 2; // 2dp margin on each side for symmetry
            int iconWidth = logoSize + spacing + dotsSize;
            int totalWidth = iconWidth + (horizontalMargin * 2);
            int totalHeight = Math.max(logoSize, dotsSize);

            // Convert dp to pixels
            float density = mActivity.getResources().getDisplayMetrics().density;
            int logoSizePx = (int) (logoSize * density);
            int dotsSizePx = (int) (dotsSize * density);
            int spacingPx = (int) (spacing * density);
            int horizontalMarginPx = (int) (horizontalMargin * density);
            int totalWidthPx = (int) (totalWidth * density);
            int totalHeightPx = (int) (totalHeight * density);

            // Create a bitmap to draw on
            Bitmap combinedBitmap =
                    Bitmap.createBitmap(totalWidthPx, totalHeightPx, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(combinedBitmap);

            // Draw the Brave logo on the left (with left margin, centered vertically)
            int logoLeft = horizontalMarginPx;
            int logoTop = (totalHeightPx - logoSizePx) / 2;
            braveLogoDrawable.setBounds(
                    logoLeft, logoTop, logoLeft + logoSizePx, logoTop + logoSizePx);
            braveLogoDrawable.draw(canvas);

            // Draw the three dots on the right (centered vertically)
            int dotsLeft = logoLeft + logoSizePx + spacingPx;
            int dotsTop = (totalHeightPx - dotsSizePx) / 2;
            threeDotsDrawable.setBounds(
                    dotsLeft, dotsTop, dotsLeft + dotsSizePx, dotsTop + dotsSizePx);
            threeDotsDrawable.draw(canvas);

            // Create and return BitmapDrawable
            return new BitmapDrawable(mActivity.getResources(), combinedBitmap);
        } catch (Exception e) {
            // If anything goes wrong, return null and the default icon will be used
            return null;
        }
    }

    @Override
    public MenuButton getMenuButton() {
        updateMenuButtonState();
        return BottomToolbarConfiguration.isToolbarTopAnchored() && isMenuFromBottom()
                ? null
                : super.getMenuButton();
    }

    @Override
    public void drawTabSwitcherAnimationOverlay(View root, Canvas canvas, int alpha) {
        if (BottomToolbarConfiguration.isToolbarTopAnchored() && isMenuFromBottom()) return;
        super.drawTabSwitcherAnimationOverlay(root, canvas, alpha);
    }

    @Override
    public void setVisibility(boolean visible) {
        updateMenuButtonState();

        // Remove menu from top address bar if it is shown in the bottom controls.
        super.setVisibility(
                (isMenuFromBottom() && BottomToolbarConfiguration.isToolbarTopAnchored())
                        ? false
                        : visible);
    }

    private void updateMenuButtonState() {
        BraveToolbarLayoutImpl layout =
                (BraveToolbarLayoutImpl) mActivity.findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.updateMenuButtonState();
        }
    }

    public boolean isToolbarBottomAnchored() {
        return BottomToolbarConfiguration.isToolbarBottomAnchored();
    }

    public static void setMenuFromBottom(boolean isMenuFromBottom) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, isMenuFromBottom);
    }

    public static boolean isMenuFromBottom() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true);
    }

    public static void setupPropertyModel(
            MenuButton menuButton, Supplier<MenuButtonState> menuButtonStateSupplier) {
        PropertyModel menuButtonPropertyModel =
                new PropertyModel.Builder(MenuButtonProperties.ALL_KEYS)
                        .with(MenuButtonProperties.STATE_SUPPLIER, menuButtonStateSupplier)
                        .build();
        PropertyModelChangeProcessor.create(
                menuButtonPropertyModel, menuButton, new MenuButtonViewBinder());
    }
}
