/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeUtils;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.ui.edge_to_edge.EdgeToEdgeSystemBarColorHelper;
import org.chromium.ui.util.ColorUtils;

import java.util.HashMap;
import java.util.Map;

@NullMarked
public class BraveSettingsActivity extends SettingsActivity {

    private @Nullable View mContentView;

    /**
     * Original bottom padding for each Settings content view.
     *
     * <p>Used as the baseline when applying navigation-bar insets so repeated updates do not
     * accumulate padding.
     */
    private final Map<View, Integer> mOriginalContentBottomPaddings = new HashMap<>();

    private final FragmentManager.FragmentLifecycleCallbacks mSettingsFragmentLifecycleCallbacks =
            new FragmentManager.FragmentLifecycleCallbacks() {
                @Override
                public void onFragmentViewCreated(
                        FragmentManager fragmentManager,
                        Fragment fragment,
                        View view,
                        @Nullable Bundle savedInstanceState) {
                    view.post(() -> onSettingsFragmentViewReady(fragment, view));
                }

                @Override
                public void onFragmentViewDestroyed(
                        FragmentManager fragmentManager, Fragment fragment) {
                    View fragmentView = fragment.getView();
                    if (fragmentView == null) return;

                    View insetView = getInsetView(fragment, fragmentView);
                    if (insetView == null) return;

                    mOriginalContentBottomPaddings.remove(insetView);
                }
            };

    @Override
    protected boolean shouldDrawEdgeToEdgeOnCreate() {
        // Settings applies its own navigation-bar inset, so it can always draw behind the bar.
        return true;
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        getSupportFragmentManager()
                .registerFragmentLifecycleCallbacks(
                        mSettingsFragmentLifecycleCallbacks, /* recursive= */ true);
        super.onCreate(savedInstanceState);

        // Settings opts into edge-to-edge before the global rollout, so initialize its transparent
        // navigation bar here when the base activity has not done so.
        if (!EdgeToEdgeUtils.isEdgeToEdgeEverywhereEnabled()) {
            initializeSystemBarColors(
                    assumeNonNull(getEdgeToEdgeManager()).getEdgeToEdgeSystemBarColorHelper());
        }

        View contentView = getContentView();
        mContentView = contentView;
        contentView.setBackgroundColor(SemanticColorUtils.getSettingsBackgroundColor(this));
        ViewCompat.setOnApplyWindowInsetsListener(contentView, this::updateSettingsContentInsets);
        ViewCompat.requestApplyInsets(contentView);
    }

    @Override
    protected void onDestroy() {
        getSupportFragmentManager()
                .unregisterFragmentLifecycleCallbacks(mSettingsFragmentLifecycleCallbacks);
        if (mContentView != null) {
            ViewCompat.setOnApplyWindowInsetsListener(mContentView, null);
            mContentView = null;
        }
        mOriginalContentBottomPaddings.clear();
        super.onDestroy();
    }

    @Override
    protected void initializeSystemBarColors(
            EdgeToEdgeSystemBarColorHelper edgeToEdgeSystemBarColorHelper) {
        super.initializeSystemBarColors(edgeToEdgeSystemBarColorHelper);

        // Retain the resolved RGB value so the system UI elements on the navigation bar use the
        // correct contrast.
        int transparentNavigationBarColor =
                ColorUtils.setAlphaComponent(
                        edgeToEdgeSystemBarColorHelper.getNavigationBarColor(), /* alpha= */ 0);
        edgeToEdgeSystemBarColorHelper.setNavigationBarColor(transparentNavigationBarColor);
        edgeToEdgeSystemBarColorHelper.setNavigationBarDividerColor(Color.TRANSPARENT);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.clear();
        getMenuInflater().inflate(R.menu.exit_settings_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.removeItem(R.id.menu_id_general_help);
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.close_menu_id) {
            Intent intent = new Intent(this, ChromeTabbedActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            intent.setAction(Intent.ACTION_VIEW);
            startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void onSettingsFragmentViewReady(Fragment fragment, View fragmentView) {
        if (!fragment.isAdded() || fragment.getView() != fragmentView) return;
        if (isFinishing() || isDestroyed()) return;

        View insetView = getInsetView(fragment, fragmentView);
        if (insetView == null) return;

        mOriginalContentBottomPaddings.putIfAbsent(insetView, insetView.getPaddingBottom());
        if (insetView instanceof RecyclerView recyclerView) {
            recyclerView.setClipToPadding(false);
        }

        View contentView = mContentView;
        if (contentView == null) return;

        WindowInsetsCompat rootWindowInsets = ViewCompat.getRootWindowInsets(contentView);
        if (rootWindowInsets == null) return;

        updateSettingsContentInsets(contentView, rootWindowInsets);
    }

    private WindowInsetsCompat updateSettingsContentInsets(
            View contentView, WindowInsetsCompat windowInsets) {
        if (isFinishing() || isDestroyed()) return windowInsets;

        // The nested Settings content view can receive consumed insets. Read the root values so
        // a later dispatch cannot clear the taskbar padding.
        WindowInsetsCompat rootWindowInsets = ViewCompat.getRootWindowInsets(contentView);
        if (rootWindowInsets == null) rootWindowInsets = windowInsets;

        setBottomPadding(
                contentView, rootWindowInsets.getInsets(WindowInsetsCompat.Type.ime()).bottom);

        Insets navigationBarInsets =
                rootWindowInsets.getInsets(WindowInsetsCompat.Type.navigationBars());
        Insets tappableElementInsets =
                rootWindowInsets.getInsets(WindowInsetsCompat.Type.tappableElement());
        int navigationBarBottomInset =
                Math.max(navigationBarInsets.bottom, tappableElementInsets.bottom);
        for (Map.Entry<View, Integer> entry : mOriginalContentBottomPaddings.entrySet()) {
            setBottomPadding(entry.getKey(), entry.getValue() + navigationBarBottomInset);
        }
        return windowInsets;
    }

    private static @Nullable View getInsetView(Fragment fragment, View fragmentView) {
        if (fragment instanceof BottomInsetViewProvider provider) {
            return provider.getBottomInsetView(fragmentView);
        }

        RecyclerView recyclerView = fragmentView.findViewById(R.id.recycler_view);
        return recyclerView;
    }

    private static void setBottomPadding(View view, int bottomPadding) {
        if (view.getPaddingBottom() == bottomPadding) return;

        view.setPadding(
                view.getPaddingLeft(), view.getPaddingTop(), view.getPaddingRight(), bottomPadding);
    }
}
