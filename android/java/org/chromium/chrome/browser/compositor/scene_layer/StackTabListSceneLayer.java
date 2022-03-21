/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.scene_layer;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.RectF;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.compositor.LayerTitleCache;
import org.chromium.chrome.browser.compositor.layouts.Layout;
import org.chromium.chrome.browser.compositor.layouts.components.LayoutTab;
import org.chromium.chrome.browser.compositor.layouts.components.StackLayoutTab;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.layouts.scene_layer.SceneLayer;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.ui.resources.ResourceManager;
import org.chromium.ui.util.ColorUtils;

@JNINamespace("android")
public class StackTabListSceneLayer extends TabListSceneLayer {
    // To delete in bytecode, members from parent class will be used instead.
    private long mNativePtr;
    private boolean mIsInitialized;

    public void initStack(LayerTitleCache layerTitleCache) {
        if (mNativePtr == 0 || mIsInitialized || layerTitleCache == null) return;
        StackTabListSceneLayerJni.get().setStackDependencies(
                mNativePtr, StackTabListSceneLayer.this, layerTitleCache);
    }

    @Override
    public void pushLayers(Context context, RectF viewport, RectF contentViewport, Layout layout,
            TabContentManager tabContentManager, ResourceManager resourceManager,
            BrowserControlsStateProvider browserControls, int backgroundResourceId,
            float backgroundAlpha, int backgroundTopOffset) {
        pushStackLayers(context, viewport, contentViewport, layout, tabContentManager,
                resourceManager, browserControls, backgroundResourceId, backgroundAlpha,
                backgroundTopOffset);
        super.pushLayers(context, viewport, contentViewport, layout, tabContentManager,
                resourceManager, browserControls, backgroundResourceId, backgroundAlpha,
                backgroundTopOffset);
    }

    private void pushStackLayers(Context context, RectF viewport, RectF contentViewport,
            Layout layout, TabContentManager tabContentManager, ResourceManager resourceManager,
            BrowserControlsStateProvider browserControls, int backgroundResourceId,
            float backgroundAlpha, int backgroundTopOffset) {
        if (mNativePtr == 0) return;

        Resources res = context.getResources();
        final float dpToPx = res.getDisplayMetrics().density;
        final int tabListBgColor = getTabListBackgroundColor(context);

        LayoutTab[] tabs = layout.getLayoutTabsToRender();
        int tabsCount = tabs != null ? tabs.length : 0;

        if (!mIsInitialized) {
            init(tabContentManager, resourceManager);
        }

        TabListSceneLayerJni.get().updateLayer(mNativePtr, StackTabListSceneLayer.this,
                tabListBgColor, viewport.left, viewport.top, viewport.width(), viewport.height());

        if (backgroundResourceId != INVALID_RESOURCE_ID) {
            TabListSceneLayerJni.get().putBackgroundLayer(mNativePtr, StackTabListSceneLayer.this,
                    backgroundResourceId, backgroundAlpha, backgroundTopOffset);
        }

        for (int i = 0; i < tabsCount; i++) {
            StackLayoutTab t = (StackLayoutTab) tabs[i];
            assert t.isVisible() : "LayoutTab in that list should be visible";
            final float decoration = t.getDecorationAlpha();

            boolean useIncognitoColors = t.isIncognito();

            int defaultThemeColor = ChromeColors.getDefaultThemeColor(context, useIncognitoColors);

            StackTabListSceneLayerJni.get().putStackTabLayer(mNativePtr,
                    StackTabListSceneLayer.this, t.getId(), R.drawable.btn_delete_24dp,
                    t.isCloseButtonOnRight(), t.getTiltXPivotOffset() * dpToPx,
                    t.getTiltYPivotOffset() * dpToPx, t.getTiltX(), t.getTiltY(),
                    t.getBorderCloseButtonAlpha() * decoration,
                    StackLayoutTab.CLOSE_BUTTON_WIDTH_DP * dpToPx,
                    res.getDimensionPixelSize(R.dimen.tab_switcher_close_button_size),
                    useIncognitoColors ? Color.WHITE
                                       : SemanticColorUtils.getDefaultIconColorSecondary(context),
                    t.isTitleNeeded(), R.drawable.tabswitcher_border_frame, t.isIncognito(),
                    t.getRenderX() * dpToPx, t.getRenderY() * dpToPx,
                    t.getScaledContentWidth() * dpToPx, t.getOriginalContentWidth() * dpToPx,
                    t.getOriginalContentHeight() * dpToPx, t.getAlpha(),
                    t.getBorderAlpha() * decoration, t.getBorderScale(), defaultThemeColor,
                    t.insetBorderVertical());
        }
    }

    @Override
    protected void initializeNative() {
        if (mNativePtr == 0) {
            mNativePtr = StackTabListSceneLayerJni.get().init(StackTabListSceneLayer.this);
        }
        assert mNativePtr != 0;
    }

    @Override
    public void destroy() {
        super.destroy();
        mNativePtr = 0;
    }

    @NativeMethods
    interface Natives {
        long init(StackTabListSceneLayer caller);
        void putStackTabLayer(long nativeStackTabListSceneLayer, StackTabListSceneLayer caller,
                int selectedId, int closeButtonResourceId, boolean isPortrait, float pivotX,
                float pivotY, float rotationX, float rotationY, float closeAlpha,
                float closeBtnWidth, float closeBtnAssetSize, int closeButtonColor,
                boolean showTabTitle, int borderResourceId, boolean incognito, float x, float y,
                float width, float contentWidth, float contentHeight, float alpha,
                float borderAlpha, float borderScale, int defaultThemeColor,
                boolean insetVerticalBorder);
        void setStackDependencies(long nativeStackTabListSceneLayer, StackTabListSceneLayer caller,
                LayerTitleCache layerTitleCache);
    }
}
