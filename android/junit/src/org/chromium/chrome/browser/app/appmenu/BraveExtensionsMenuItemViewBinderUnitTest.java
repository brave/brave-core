/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.appmenu;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ui.appmenu.AppMenuClickHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

/**
 * Unit tests for {@link BraveExtensionsMenuItemViewBinder}, the binder for the phone app menu's
 * Extensions item. Checks that the subtitle only shows when set (used on internal pages) and that a
 * disabled item ignores clicks.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveExtensionsMenuItemViewBinderUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private AppMenuClickHandler mClickHandler;

    private View mView;
    private PropertyModel mModel;

    @Before
    public void setUp() {
        Context context =
                new ContextThemeWrapper(
                        ContextUtils.getApplicationContext(), R.style.Theme_BrowserUI_DayNight);
        mView = LayoutInflater.from(context).inflate(R.layout.brave_extensions_menu_item, null);
        mModel =
                new PropertyModel.Builder(BraveExtensionsMenuItemViewBinder.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.brave_extensions_menu_id)
                        .with(AppMenuItemProperties.TITLE, "Extensions")
                        .with(AppMenuItemProperties.ENABLED, true)
                        .with(AppMenuItemProperties.CLICK_HANDLER, mClickHandler)
                        .build();
        PropertyModelChangeProcessor.create(mModel, mView, BraveExtensionsMenuItemViewBinder::bind);
    }

    @Test
    public void testSubtitle_HiddenWhenUnset() {
        TextView subtitle = mView.findViewById(R.id.menu_item_subtitle);
        assertEquals(View.GONE, subtitle.getVisibility());
    }

    @Test
    public void testSubtitle_ShownWhenSet() {
        mModel.set(BraveExtensionsMenuItemViewBinder.SUBTITLE, "Unavailable");

        TextView subtitle = mView.findViewById(R.id.menu_item_subtitle);
        assertEquals(View.VISIBLE, subtitle.getVisibility());
        assertEquals("Unavailable", subtitle.getText().toString());
    }

    @Test
    public void testEnabled_ClickForwardsToHandler() {
        assertTrue(mView.isClickable());
        mView.performClick();
        verify(mClickHandler).onItemClick(mModel);
    }

    @Test
    public void testDisabled_ClickIgnored() {
        mModel.set(AppMenuItemProperties.ENABLED, false);

        assertFalse(mView.isClickable());
        mView.performClick();
        verify(mClickHandler, never()).onItemClick(any());
    }
}
