/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.lens.LensController;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.permissions.PermissionCallback;

import java.util.function.BooleanSupplier;

public class BraveLocationBarMediator extends LocationBarMediator {
    private WindowAndroid mWindowAndroid;
    private LocationBarLayout mLocationBarLayout;
    private boolean mIsUrlFocusChangeInProgress;
    private boolean mUrlHasFocus;
    private boolean mIsTablet;
    private boolean mNativeInitialized;
    private boolean mIsLocationBarFocusedFromNtpScroll;
    private Context mContext;
    private @BrandedColorScheme int mBrandedColorScheme = BrandedColorScheme.APP_DEFAULT;

    public BraveLocationBarMediator(
            @NonNull Context context,
            @NonNull LocationBarLayout locationBarLayout,
            @NonNull LocationBarDataProvider locationBarDataProvider,
            @NonNull LocationBarEmbedderUiOverrides embedderUiOverrides,
            @NonNull ObservableSupplier<Profile> profileSupplier,
            @NonNull OverrideUrlLoadingDelegate overrideUrlLoadingDelegate,
            @NonNull LocaleManager localeManager,
            @NonNull OneshotSupplier<TemplateUrlService> templateUrlServiceSupplier,
            @NonNull BackKeyBehaviorDelegate backKeyBehavior,
            @NonNull WindowAndroid windowAndroid,
            boolean isTablet,
            @NonNull LensController lensController,
            @NonNull SaveOfflineButtonState saveOfflineButtonState,
            @NonNull OmniboxUma omniboxUma,
            @NonNull BooleanSupplier isToolbarMicEnabledSupplier,
            @NonNull OmniboxSuggestionsDropdownEmbedderImpl dropdownEmbedder,
            @Nullable ObservableSupplier<TabModelSelector> tabModelSelectorSupplier) {
        super(
                context,
                locationBarLayout,
                locationBarDataProvider,
                embedderUiOverrides,
                profileSupplier,
                overrideUrlLoadingDelegate,
                localeManager,
                templateUrlServiceSupplier,
                backKeyBehavior,
                windowAndroid,
                isTablet,
                lensController,
                saveOfflineButtonState,
                omniboxUma,
                isToolbarMicEnabledSupplier,
                dropdownEmbedder,
                tabModelSelectorSupplier);
    }

    public static Class<OmniboxUma> getOmniboxUmaClass() {
        return OmniboxUma.class;
    }

    public static Class<SaveOfflineButtonState> getSaveOfflineButtonStateClass() {
        return SaveOfflineButtonState.class;
    }

    public static Class<LensController> getLensControllerClass() {
        return LensController.class;
    }

    public static Class<LocaleManager> getLocaleManagerClass() {
        return LocaleManager.class;
    }

    public static Class<PrivacyPreferencesManager> getPrivacyPreferencesManagerClass() {
        return PrivacyPreferencesManager.class;
    }

    public static Class<OmniboxSuggestionsDropdownEmbedderImpl>
    getOmniboxSuggestionsDropdownEmbedderImplClass() {
        return OmniboxSuggestionsDropdownEmbedderImpl.class;
    }

    @Override
    void updateButtonVisibility() {
        super.updateButtonVisibility();
        updateQRButtonVisibility();
    }

    @Override
    public void onPrimaryColorChanged() {
        super.onPrimaryColorChanged();
        updateQRButtonColors();
    }

    void updateQRButtonColors() {
        if (mLocationBarLayout instanceof BraveLocationBarLayout) {
            ((BraveLocationBarLayout) mLocationBarLayout)
                    .setQRButtonTint(
                            ThemeUtils.getThemedToolbarIconTint(mContext, mBrandedColorScheme));
        }
    }

    private void updateQRButtonVisibility() {
        if (mLocationBarLayout instanceof BraveLocationBarLayout) {
            ((BraveLocationBarLayout) mLocationBarLayout)
                    .setQRButtonVisibility(shouldShowQRButton());
        }
    }

    private boolean shouldShowQRButton() {
        if (!mNativeInitialized) {
            return false;
        }
        if (mIsTablet) {
            return mUrlHasFocus || mIsUrlFocusChangeInProgress;
        } else {
            return !mIsTablet && !shouldShowDeleteButton()
                    && (mUrlHasFocus || mIsUrlFocusChangeInProgress
                            || mIsLocationBarFocusedFromNtpScroll);
        }
    }

    protected boolean shouldShowDeleteButton() {
        assert false;
        return false;
    }

    void qrButtonClicked(View view) {
        if (!mNativeInitialized) return;
        if (ensureCameraPermission()) {
            openQRCodeDialog();
        }
    }

    private boolean ensureCameraPermission() {
        if (mWindowAndroid.hasPermission(Manifest.permission.CAMERA)) {
            return true;
        }

        PermissionCallback callback = (permissions, grantResults) -> {
            if (grantResults.length != 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openQRCodeDialog();
            }
        };

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mWindowAndroid.requestPermissions(new String[] {Manifest.permission.CAMERA}, callback);
        }

        return false;
    }

    private void openQRCodeDialog() {
        if (mContext != null && mContext instanceof AppCompatActivity) {
            BraveLocationBarQRDialogFragment braveLocationBarQRDialogFragment =
                    BraveLocationBarQRDialogFragment.newInstance(this);
            braveLocationBarQRDialogFragment.setCancelable(false);
            braveLocationBarQRDialogFragment.show(
                    ((AppCompatActivity) mContext).getSupportFragmentManager(),
                    "BraveLocationBarQRDialogFragment");
        }
    }
}
