/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.os.Build;
import android.view.ContextThemeWrapper;
import android.widget.ImageView;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.browser_ui.styles.R;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;

/** Unit tests for {@link BraveSettingsIconTintUtils}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = Build.VERSION_CODES.S)
public class BraveSettingsIconTintUtilsTest {
    private static final int[] DISABLED_STATE_SET = new int[] {-android.R.attr.state_enabled};
    private static final int[] ENABLED_STATE_SET = new int[] {android.R.attr.state_enabled};
    private static final int[] DEFAULT_STATE_SET = new int[] {};

    @Test
    public void testApplyIconTint_dynamicColorsDisabled_usesDefaultTint() {
        assertUsesDefaultTint();
    }

    @Test
    public void testApplyIconTint_dynamicColorsEnabled_preservesDisabledTint() {
        ColorStateList defaultTint = createDefaultTint();
        Context context =
                new DefaultTintContext(
                        new ContextThemeWrapper(
                                ApplicationProvider.getApplicationContext(),
                                R.style.Theme_MaterialComponents_DayNight),
                        defaultTint);
        TintCapturingImageView icon = new TintCapturingImageView(context);

        BraveSettingsIconTintUtils.applyIconTint(icon, true);

        ColorStateList actualTint = icon.getCapturedTint();
        assertNotNull(actualTint);
        assertEquals(
                defaultTint.getColorForState(DISABLED_STATE_SET, defaultTint.getDefaultColor()),
                actualTint.getColorForState(DISABLED_STATE_SET, actualTint.getDefaultColor()));
        assertEquals(
                SemanticColorUtils.getDefaultIconColorAccent1(context),
                actualTint.getColorForState(ENABLED_STATE_SET, actualTint.getDefaultColor()));
    }

    @Test
    public void testClearIconTint_removesPreviouslyAppliedTint() {
        Context context =
                new DefaultTintContext(
                        ApplicationProvider.getApplicationContext(), createDefaultTint());
        TintCapturingImageView icon = new TintCapturingImageView(context);
        BraveSettingsIconTintUtils.applyIconTint(icon, false);
        icon.setColorFilter(0xffaabbcc);

        assertNotNull(icon.getCapturedTint());
        assertNotNull(icon.getColorFilter());

        BraveSettingsIconTintUtils.clearIconTint(icon);

        assertNull(icon.getCapturedTint());
        assertNull(icon.getColorFilter());
    }

    private void assertUsesDefaultTint() {
        ColorStateList expectedTint = createDefaultTint();
        Context context =
                new DefaultTintContext(ApplicationProvider.getApplicationContext(), expectedTint);
        TintCapturingImageView icon = new TintCapturingImageView(context);

        BraveSettingsIconTintUtils.applyIconTint(icon, false);

        assertSame(expectedTint, icon.getCapturedTint());
    }

    private static ColorStateList createDefaultTint() {
        return new ColorStateList(
                new int[][] {DISABLED_STATE_SET, DEFAULT_STATE_SET},
                new int[] {0xffaabbcc, 0xff112233});
    }

    private static class DefaultTintContext extends ContextWrapper {
        private final Resources mResources;

        DefaultTintContext(Context base, ColorStateList defaultTint) {
            super(base);
            mResources = new DefaultTintResources(base.getResources(), defaultTint);
        }

        @Override
        public Resources getResources() {
            return mResources;
        }
    }

    private static class DefaultTintResources extends Resources {
        private final ColorStateList mDefaultTint;

        DefaultTintResources(Resources base, ColorStateList defaultTint) {
            super(base.getAssets(), base.getDisplayMetrics(), base.getConfiguration());
            mDefaultTint = defaultTint;
        }

        @Override
        public ColorStateList getColorStateList(int resId) {
            if (resId == R.color.default_icon_color_tint_list) {
                return mDefaultTint;
            }
            return super.getColorStateList(resId);
        }

        @Override
        public ColorStateList getColorStateList(int resId, Theme theme) {
            if (resId == R.color.default_icon_color_tint_list) {
                return mDefaultTint;
            }
            return super.getColorStateList(resId, theme);
        }
    }

    private static class TintCapturingImageView extends ImageView {
        private ColorStateList mTint;

        TintCapturingImageView(Context context) {
            super(context);
        }

        @Override
        public void setImageTintList(ColorStateList tint) {
            super.setImageTintList(tint);
            mTint = tint;
        }

        ColorStateList getCapturedTint() {
            return mTint;
        }
    }
}
