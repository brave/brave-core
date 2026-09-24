/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bottombar;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.lifetime.Destroyable;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.actions.ActionId;
import org.chromium.chrome.browser.ui.actions.ActionProperties;
import org.chromium.chrome.browser.ui.actions.ActionRegistry;
import org.chromium.chrome.browser.ui.actions.ActionUtils;
import org.chromium.chrome.browser.ui.actions.ResourceTextResolver;
import org.chromium.chrome.browser.ui.actions.button.ButtonState;
import org.chromium.components.omnibox.OmniboxFocusReason;
import org.chromium.ui.modelutil.PropertyModel;

/**
 * Fills in what the two buttons Brave adds to the bottom bar look like and do: the bookmark button,
 * and the search accelerator that holds the centre slot.
 *
 * <p>{@code BraveBottomBarCoordinator} is what puts them in their slots. The home and new tab
 * buttons that share the home slot are both upstream's, and keep upstream's icons and behaviour.
 */
@NullMarked
public class BraveBottomBarActionCoordinator implements Destroyable {
    private static final String TAG = "BraveBottomBarAction";

    private final ActionRegistry mActionRegistry;
    private final ActivityTabProvider mTabProvider;
    private final PropertyModel mBookmarkActionModel;

    /**
     * @param actionRegistry The registry holding the bottom bar's action models.
     * @param tabProvider Provides the tab the bookmark button acts on.
     * @param setUrlBarFocusAction Focuses the address bar, taking an {@link OmniboxFocusReason}.
     */
    public BraveBottomBarActionCoordinator(
            ActionRegistry actionRegistry,
            ActivityTabProvider tabProvider,
            Callback<Integer> setUrlBarFocusAction) {
        mActionRegistry = actionRegistry;
        mTabProvider = tabProvider;

        ActionUtils.registerAction(
                actionRegistry,
                ActionId.BRAVE_BOOKMARK,
                ActionProperties.ALL_KEYS,
                R.drawable.btn_bookmark,
                R.string.accessibility_menu_bookmark,
                R.string.accessibility_menu_bookmark);
        PropertyModel bookmarkActionModel = actionRegistry.get(ActionId.BRAVE_BOOKMARK).get();
        assert bookmarkActionModel != null : "Bookmark action should be registered";
        mBookmarkActionModel = bookmarkActionModel;
        mBookmarkActionModel.set(ActionProperties.ON_PRESS_CALLBACK, v -> addOrEditBookmark());

        ActionUtils.registerAction(
                actionRegistry,
                ActionId.BRAVE_SEARCH,
                ActionProperties.BASE_KEYS,
                R.drawable.ic_search_21dp,
                R.string.accessibility_toolbar_btn_search_accelerator,
                R.string.accessibility_toolbar_btn_search_accelerator);
        PropertyModel searchActionModel = actionRegistry.get(ActionId.BRAVE_SEARCH).get();
        assert searchActionModel != null : "Search action should be registered";
        searchActionModel.set(
                ActionProperties.ON_PRESS_CALLBACK,
                v -> setUrlBarFocusAction.onResult(OmniboxFocusReason.ACCELERATOR_TAP));
    }

    /**
     * Mirrors onto the bottom bar what the top toolbar knows about the current tab's bookmark.
     *
     * @param isBookmarked Whether the current tab is bookmarked.
     * @param editingAllowed Whether the bookmark can be added or edited.
     */
    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        mBookmarkActionModel.set(
                ActionProperties.ICON_ID,
                isBookmarked ? R.drawable.btn_bookmark_fill : R.drawable.btn_bookmark);
        mBookmarkActionModel.set(
                ActionProperties.CONTENT_DESCRIPTION_RESOLVER,
                new ResourceTextResolver(
                        isBookmarked
                                ? R.string.edit_bookmark
                                : R.string.accessibility_menu_bookmark));
        mBookmarkActionModel.set(
                ActionProperties.BUTTON_STATE,
                editingAllowed ? ButtonState.DEFAULT : ButtonState.UNCLICKABLE);
    }

    private void addOrEditBookmark() {
        Tab tab = mTabProvider.get();
        if (tab == null) return;
        try {
            BraveActivity.getBraveActivity().addOrEditBookmark(tab);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "addOrEditBookmark " + e);
        }
    }

    @Override
    public void destroy() {
        mActionRegistry.unregister(ActionId.BRAVE_BOOKMARK);
        mActionRegistry.unregister(ActionId.BRAVE_SEARCH);
    }
}
