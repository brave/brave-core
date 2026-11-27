/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.extensions;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.annotation.StringRes;

import com.google.android.material.color.MaterialColors;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.theme.BraveDynamicColors;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTask;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTaskFeature;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTaskFeatureKey;
import org.chromium.chrome.browser.ui.extensions.ExtensionAction;
import org.chromium.chrome.browser.ui.extensions.ExtensionActionPopupContents;
import org.chromium.chrome.browser.ui.extensions.ExtensionsToolbarBridge;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetContent;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetType;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.display.DisplayAndroid;
import org.chromium.ui.display.DisplayUtil;

import java.util.function.Supplier;

/** Phone bottom sheet listing the installed extensions, opened from the app menu. */
@NullMarked
public class BraveExtensionsMenuCoordinator implements ChromeAndroidTaskFeature {
    private static final BottomSheetType SHEET_TYPE =
            new BottomSheetType.Builder().setUserInitiated(true).build();

    private final Activity mActivity;
    private final ActivityWindowAndroid mWindowAndroid;
    private final BottomSheetController mBottomSheetController;
    private final Supplier<@Nullable Tab> mTabSupplier;
    private final ExtensionsToolbarBridge mBridge;
    private final ExtensionsToolbarBridge.Observer mBridgeObserver =
            new ExtensionsToolbarBridge.Observer() {
                @Override
                public void onActionsInitialized() {
                    updateList();
                }

                @Override
                public void onActionAdded(String actionId) {
                    updateList();
                }

                @Override
                public void onActionRemoved(String actionId) {
                    updateList();
                }

                @Override
                public void onActionUpdated(String actionId) {
                    updateList();
                }
            };

    private @Nullable SheetContent mSheetContent;

    /** Shows the sheet, creating the coordinator as a feature of {@code task} if needed. */
    public static void show(
            Activity activity,
            ChromeAndroidTask task,
            Profile profile,
            ActivityWindowAndroid windowAndroid,
            BottomSheetController bottomSheetController,
            Supplier<@Nullable Tab> tabSupplier) {
        // Registered as a task feature so it's destroyed before the profile and the native
        // browser window it depends on.
        BraveExtensionsMenuCoordinator coordinator =
                (BraveExtensionsMenuCoordinator)
                        task.addFeature(
                                new ChromeAndroidTaskFeatureKey(
                                        BraveExtensionsMenuCoordinator.class,
                                        profile,
                                        windowAndroid),
                                () ->
                                        new BraveExtensionsMenuCoordinator(
                                                activity,
                                                task,
                                                profile,
                                                windowAndroid,
                                                bottomSheetController,
                                                tabSupplier));
        if (coordinator != null) {
            coordinator.showSheet();
        }
    }

    private BraveExtensionsMenuCoordinator(
            Activity activity,
            ChromeAndroidTask task,
            Profile profile,
            ActivityWindowAndroid windowAndroid,
            BottomSheetController bottomSheetController,
            Supplier<@Nullable Tab> tabSupplier) {
        mActivity = activity;
        mWindowAndroid = windowAndroid;
        mBottomSheetController = bottomSheetController;
        mTabSupplier = tabSupplier;
        mBridge = new ExtensionsToolbarBridge(task, profile);
        mBridge.addObserver(mBridgeObserver);
        // Native requires both delegates. Popups aren't supported here yet.
        mBridge.setActionListDelegate(
                new ExtensionsToolbarBridge.ActionListDelegate() {
                    @Override
                    public void triggerPopup(
                            String actionId,
                            ExtensionActionPopupContents popupContents,
                            boolean inspectWithDevTools) {
                        popupContents.destroy();
                    }

                    @Override
                    public void showContextMenu(String actionId) {}

                    @Override
                    public boolean hasPoppedOutAction() {
                        return false;
                    }

                    @Override
                    public void hideActivePopup() {}

                    @Override
                    public boolean hasActivePopup() {
                        return false;
                    }
                });
        mBridge.setMenuDelegate(this::dismissSheet);
    }

    @Override
    public void onAddedToTask(InitInfo initInfo) {}

    @Override
    public void onFeatureRemoved() {
        if (mSheetContent != null) {
            mBottomSheetController.hideContent(mSheetContent, /* animate= */ false);
        }
        mBridge.setActionListDelegate(null);
        mBridge.setMenuDelegate(null);
        mBridge.removeObserver(mBridgeObserver);
        mBridge.destroy();
    }

    private void showSheet() {
        if (mSheetContent != null) return;

        // The layout colors resolve against the dynamic color overlay when it's on.
        Context context =
                BraveDynamicColors.isDynamicColorsEnabled()
                        ? mActivity
                        : new ContextThemeWrapper(
                                mActivity, R.style.BraveExtensionsMenuSheetStaticColors);
        FrameLayout view = new SheetContainerView(context);
        LayoutInflater.from(context).inflate(R.layout.brave_extensions_menu_sheet, view, true);
        SheetContent content = new SheetContent(view);
        mSheetContent = content;
        updateList();
        mBottomSheetController.requestShowContent(content, /* animate= */ true);
    }

    private void dismissSheet() {
        if (mSheetContent != null) {
            mBottomSheetController.hideContent(mSheetContent, /* animate= */ true);
        }
    }

    private void updateList() {
        if (mSheetContent == null) return;
        View content = mSheetContent.getContentView();

        ViewGroup list = content.findViewById(R.id.extensions_menu_list);
        list.removeAllViews();
        Tab tab = mTabSupplier.get();
        WebContents webContents = tab != null ? tab.getWebContents() : null;
        LayoutInflater inflater = LayoutInflater.from(content.getContext());
        Resources res = mActivity.getResources();
        // Same canvas spec as the toolbar's ExtensionActionIconUtil, which isn't visible here.
        DisplayAndroid display = mWindowAndroid.getDisplay();
        int iconSize =
                res.getDimensionPixelSize(R.dimen.brave_extensions_menu_sheet_icon_canvas_size);
        int iconSizeDp = DisplayUtil.pxToDp(display, iconSize);
        float scaleFactor = display.getDipScale();
        for (String actionId : mBridge.getAllActionIds()) {
            ExtensionAction action = mBridge.getAction(actionId, webContents);
            if (action == null) continue;

            TextView row =
                    (TextView)
                            inflater.inflate(
                                    R.layout.brave_extensions_menu_sheet_item, list, false);
            row.setText(action.getName());
            Bitmap icon =
                    mBridge.getIcon(actionId, webContents, iconSizeDp, iconSizeDp, scaleFactor);
            if (icon != null) {
                // Sized to the canvas the icon is rendered at.
                BitmapDrawable drawable = new BitmapDrawable(res, icon);
                drawable.setBounds(0, 0, iconSize, iconSize);
                row.setCompoundDrawablesRelative(drawable, null, null, null);
            }
            list.addView(row);
        }
    }

    /**
     * The sheet measures its content with an exact height when laid out. This turns it into an
     * at-most height so the list only shrinks to fit.
     */
    private static class SheetContainerView extends FrameLayout {
        SheetContainerView(Context context) {
            super(context);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.EXACTLY) {
                heightMeasureSpec =
                        MeasureSpec.makeMeasureSpec(
                                MeasureSpec.getSize(heightMeasureSpec), MeasureSpec.AT_MOST);
            }
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }

    private class SheetContent implements BottomSheetContent {
        private final View mContentView;
        private final View mScrollView;
        private final int mBackgroundColor;

        SheetContent(View contentView) {
            mContentView = contentView;
            mScrollView = contentView.findViewById(R.id.extensions_menu_scroll_view);
            mBackgroundColor =
                    MaterialColors.getColor(contentView, R.attr.colorSurfaceContainerLow);
        }

        @Override
        public View getContentView() {
            return mContentView;
        }

        @Override
        public @Nullable View getToolbarView() {
            return null;
        }

        @Override
        public int getVerticalScrollOffset() {
            return mScrollView.getScrollY();
        }

        @Override
        public void destroy() {
            // Called once the sheet is dismissed.
            if (mSheetContent != this) return;
            mSheetContent = null;
        }

        @Override
        public BottomSheetType getSheetType() {
            return SHEET_TYPE;
        }

        @Override
        public boolean swipeToDismissEnabled() {
            return true;
        }

        @Override
        public float getHalfHeightRatio() {
            return HeightMode.DISABLED;
        }

        @Override
        public float getFullHeightRatio() {
            return HeightMode.WRAP_CONTENT;
        }

        @Override
        public int getSheetBackgroundColorOverride() {
            return mBackgroundColor;
        }

        @Override
        public boolean showHandlebar() {
            return true;
        }

        // Announces the sheet by its title, as upstream does for several sheets.
        @Override
        public @StringRes int getSheetHalfHeightAccessibilityStringId() {
            return R.string.menu_extensions;
        }

        @Override
        public @StringRes int getSheetFullHeightAccessibilityStringId() {
            return R.string.menu_extensions;
        }

        @Override
        public @StringRes int getSheetClosedAccessibilityStringId() {
            return R.string.menu_extensions;
        }
    }
}
