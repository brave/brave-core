/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.theme;

import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Build;
import android.view.View;

import com.google.android.material.color.DynamicColors;
import com.google.android.material.color.DynamicColorsOptions;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.flags.ChromeFeatureMap;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.cached_flags.CachedFlag;
import org.chromium.ui.R;
import org.chromium.ui.util.AttrUtils;

/** Controls Brave's runtime use of Material dynamic colors. */
@NullMarked
public final class BraveDynamicColors {
    // ChromeCachedFlags.<clinit> creates a singleton bytecode-redirected to BraveCachedFlags
    // before BraveCachedFlags' static fields are ready, so this CachedFlag must live separately.
    private static final CachedFlag sDynamicColorsDefaultFlag =
            new CachedFlag(
                    ChromeFeatureMap.getInstance(),
                    BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS_BY_DEFAULT,
                    true);

    private BraveDynamicColors() {}

    /** Returns the cached feature that supplies the default for an unset user preference. */
    public static CachedFlag getCachedDefaultFlag() {
        return sDynamicColorsDefaultFlag;
    }

    /**
     * Returns whether dynamic colors are available for this app session.
     *
     * <p>Availability requires Android 12 or later. It does not include the user's preference;
     * {@link #isDynamicColorsEnabled()} is the preferred method for runtime behavior checks.
     */
    public static boolean isDynamicColorsAvailable() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.S;
    }

    /**
     * Returns whether dynamic colors should be used at runtime.
     *
     * <p>This requires dynamic colors to be available and the user preference to be enabled. An
     * unset preference uses the cached remotely controlled default.
     */
    public static boolean isDynamicColorsEnabled() {
        return isDynamicColorsAvailable() && isDynamicColorsUserEnabled();
    }

    /** Returns the persisted user preference or the cached default when it is unset. */
    private static boolean isDynamicColorsUserEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED,
                        getCachedDefaultFlag().isEnabled());
    }

    /**
     * Applies Material dynamic colors when they are available and enabled by the user preference.
     *
     * <p>This is a no-op otherwise.
     */
    public static void applyToActivityIfAvailable(Activity activity) {
        if (!isDynamicColorsEnabled()) {
            return;
        }

        DynamicColors.applyToActivityIfAvailable(activity);
    }

    /**
     * Applies Material dynamic colors with the supplied options when they are available and enabled
     * by the user preference.
     *
     * <p>This is a no-op otherwise.
     */
    public static void applyToActivityIfAvailable(
            Activity activity, DynamicColorsOptions dynamicColorsOptions) {
        if (!isDynamicColorsEnabled()) {
            return;
        }

        DynamicColors.applyToActivityIfAvailable(activity, dynamicColorsOptions);
    }

    /**
     * Applies the active theme's {@code globalFilledButtonBgColor} state list when dynamic colors
     * are enabled.
     *
     * <p>Call this after setting a custom filled-button background. This is a no-op when dynamic
     * colors are disabled or the theme has no filled-button color list.
     *
     * @param button view with a custom filled-button background to tint
     */
    public static void applyToFilledButtonIfEnabled(View button) {
        applyButtonBackgroundColorIfEnabled(button, R.attr.globalFilledButtonBgColor);
    }

    /**
     * Applies the active theme's {@code globalTextButtonTextColor} state list as an outlined button
     * background tint when dynamic colors are enabled.
     *
     * <p>Call this after setting a custom outlined-button background. This is a no-op when dynamic
     * colors are disabled or the theme has no text-button color list.
     *
     * @param button view with a custom outlined-button background to tint
     */
    public static void applyToOutlinedButtonIfEnabled(View button) {
        applyButtonBackgroundColorIfEnabled(button, R.attr.globalTextButtonTextColor);
    }

    /**
     * Returns the active theme's {@code globalTextButtonTextColor}, or {@code fallbackColor} when
     * dynamic colors are disabled.
     *
     * @param theme theme used to resolve {@code globalTextButtonTextColor} when dynamic colors are
     *     enabled
     * @param fallbackColor color returned when dynamic colors are disabled
     * @return the resolved theme text-button color, or {@code fallbackColor} when dynamic colors
     *     are disabled
     */
    public static int getTextButtonColor(Resources.Theme theme, int fallbackColor) {
        if (!isDynamicColorsEnabled()) {
            return fallbackColor;
        }

        return AttrUtils.resolveColor(theme, R.attr.globalTextButtonTextColor);
    }

    private static void applyButtonBackgroundColorIfEnabled(View button, int backgroundColorAttr) {
        if (!isDynamicColorsEnabled()) {
            return;
        }

        Context context = button.getContext();
        ColorStateList backgroundColor = getThemeColorStateList(context, backgroundColorAttr);
        if (backgroundColor != null) {
            button.setBackgroundTintList(backgroundColor);
        }
    }

    private static @Nullable ColorStateList getThemeColorStateList(Context context, int attr) {
        TypedArray attributes = context.obtainStyledAttributes(new int[] {attr});
        ColorStateList colors = attributes.getColorStateList(0);
        attributes.recycle();
        return colors;
    }
}
