/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts;

import android.content.Context;
import android.util.SparseArray;
import android.view.ViewGroup;

import androidx.annotation.Nullable;

import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.compositor.LayerTitleCache;
import org.chromium.chrome.browser.compositor.layouts.components.LayoutTab;
import org.chromium.chrome.browser.compositor.layouts.components.StackLayoutTab;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.compositor.layouts.phone.StackLayout;
import org.chromium.chrome.browser.compositor.layouts.phone.StackLayoutBase;
import org.chromium.chrome.browser.device.DeviceClassManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ControlContainer;
import org.chromium.chrome.browser.util.ChromeAccessibilityUtil;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.ui.resources.dynamics.DynamicResourceLoader;

import java.util.List;

public class BraveLayoutManagerChrome extends LayoutManagerChrome
        implements OverviewModeController, ChromeAccessibilityUtil.Observer {
    // To delete in bytecode, members from parent class will be used instead.
    private SparseArray<LayoutTab> mTabCache;

    // Own members.
    /** Whether to create an overview Layout when LayoutManagerChrome is created. */
    private boolean mCreateOverviewLayout;
    private LayerTitleCache mLayerTitleCache;

    public BraveLayoutManagerChrome(LayoutManagerHost host, ViewGroup contentContainer,
            boolean createOverviewLayout, @Nullable StartSurface startSurface,
            ObservableSupplier<TabContentManager> tabContentManagerSupplier,
            OneshotSupplierImpl<OverviewModeBehavior> overviewModeBehaviorSupplier,
            Supplier<TopUiThemeColorProvider> topUiThemeColorProvider, JankTracker jankTracker) {
        super(host, contentContainer, createOverviewLayout, startSurface, tabContentManagerSupplier,
                overviewModeBehaviorSupplier, topUiThemeColorProvider, jankTracker);

        mCreateOverviewLayout = createOverviewLayout && startSurface == null;
    }

    @Override
    public void init(TabModelSelector selector, TabCreatorManager creator,
            ControlContainer controlContainer, DynamicResourceLoader dynamicResourceLoader,
            TopUiThemeColorProvider topUiColorProvider) {
        if (mCreateOverviewLayout) {
            Context context = mHost.getContext();
            LayoutRenderHost renderHost = mHost.getLayoutRenderHost();
            final ObservableSupplier<? extends BrowserControlsStateProvider>
                    browserControlsSupplier =
                            BraveActivity.getBraveActivity().getBrowserControlsManagerSupplier();
            mOverviewLayout = new StackLayout(context, this, renderHost,
                    (ObservableSupplier<BrowserControlsStateProvider>) browserControlsSupplier);
        }

        super.init(selector, creator, controlContainer, dynamicResourceLoader, topUiColorProvider);

        if (DeviceClassManager.enableLayerDecorationCache()) {
            mLayerTitleCache = new LayerTitleCache(mHost.getContext(), getResourceManager());
            mLayerTitleCache.setTabModelSelector(selector);
        }
    }

    @Override
    public void hideOverview(boolean animate) {
        Layout activeLayout = getActiveLayout();
        if (activeLayout instanceof StackLayout) {
            if (animate) {
                activeLayout.onTabSelecting(time(), Tab.INVALID_TAB_ID);
            } else {
                startHiding(Tab.INVALID_TAB_ID, false);
                doneHiding();
            }
            return;
        }
        super.hideOverview(animate);
    }

    @Override
    protected void startShowing(Layout layout, boolean animate) {
        if (layout instanceof StackLayoutBase) {
            ((StackLayoutBase) layout).setLayerTitleCache(mLayerTitleCache);
        }

        super.startShowing(layout, animate);
    }

    public LayoutTab createStackLayoutTab(int id, boolean incognito, boolean showCloseButton,
            boolean isTitleNeeded, float maxContentWidth, float maxContentHeight) {
        LayoutTab tab = mTabCache.get(id);
        if (tab == null) {
            tab = new StackLayoutTab(id, incognito, mHost.getWidth(), mHost.getHeight(),
                    showCloseButton, isTitleNeeded);
            mTabCache.put(id, tab);
        } else {
            assert tab instanceof StackLayoutTab : "Wrong tab type!";
            ((StackLayoutTab) tab)
                    .initStack(mHost.getWidth(), mHost.getHeight(), showCloseButton, isTitleNeeded);
        }
        if (maxContentWidth > 0.f) tab.setMaxContentWidth(maxContentWidth);
        if (maxContentHeight > 0.f) tab.setMaxContentHeight(maxContentHeight);

        return tab;
    }

    @Override
    protected void emptyCachesExcept(int tabId) {
        super.emptyCachesExcept(tabId);
        if (mLayerTitleCache != null) mLayerTitleCache.clearExcept(tabId);
    }

    @Override
    public void initLayoutTabFromHost(final int tabId) {
        if (mLayerTitleCache != null) {
            mLayerTitleCache.remove(tabId);
        }
        super.initLayoutTabFromHost(tabId);
    }

    @Override
    public void releaseTabLayout(int id) {
        mLayerTitleCache.remove(id);
        super.releaseTabLayout(id);
    }

    @Override
    public void releaseResourcesForTab(int tabId) {
        super.releaseResourcesForTab(tabId);
        mLayerTitleCache.remove(tabId);
    }

    @Override
    public void destroy() {
        super.destroy();

        if (mLayerTitleCache != null) {
            mLayerTitleCache.shutDown();
            mLayerTitleCache = null;
        }
    }
}
