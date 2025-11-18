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

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.lens.LensController;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.omnibox.fusebox.NavigationAttachmentsCoordinator;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.components.browser_ui.accessibility.PageZoomIndicatorCoordinator;
import org.chromium.components.omnibox.AutocompleteRequestType;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.permissions.PermissionCallback;

import java.util.function.BooleanSupplier;
import java.util.function.Supplier;

@NullMarked
public class BraveLocationBarMediator extends LocationBarMediator {
    // To delete in bytecode, members from parent class will be used instead.
    private WindowAndroid mWindowAndroid;
    private LocationBarLayout mLocationBarLayout;
    private boolean mIsUrlFocusChangeInProgress;
    private boolean mUrlHasFocus;
    private boolean mIsTablet;
    private boolean mNativeInitialized;
    private boolean mIsLocationBarFocusedFromNtpScroll;
    private Context mContext;
    private OneshotSupplier<TemplateUrlService> mTemplateUrlServiceSupplier;

    private static final @BrandedColorScheme int BRANDED_COLOR_SCHEME =
            BrandedColorScheme.APP_DEFAULT;

    // The fields mWindowAndroid, mLocationBarLayout, mContext, mTemplateUrlServiceSupplier are
    // initialized at LocationBarMediator.ctor and are declared here only to be removed later
    // with asm. NullAway expects them to be initialized here, so ignore this warning.
    @SuppressWarnings("NullAway")
    public BraveLocationBarMediator(
            Context context,
            LocationBarLayout locationBarLayout,
            LocationBarDataProvider locationBarDataProvider,
            LocationBarEmbedderUiOverrides embedderUiOverrides,
            ObservableSupplier<Profile> profileSupplier,
            OverrideUrlLoadingDelegate overrideUrlLoadingDelegate,
            LocaleManager localeManager,
            OneshotSupplier<TemplateUrlService> templateUrlServiceSupplier,
            BackKeyBehaviorDelegate backKeyBehavior,
            WindowAndroid windowAndroid,
            boolean isTablet,
            LensController lensController,
            OmniboxUma omniboxUma,
            BooleanSupplier isToolbarMicEnabledSupplier,
            OmniboxSuggestionsDropdownEmbedderImpl dropdownEmbedder,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            @Nullable BrowserControlsStateProvider browserControlsStateProvider,
            Supplier<@Nullable ModalDialogManager> modalDialogManagerSupplier,
            ObservableSupplier<@AutocompleteRequestType Integer> autocompleteRequestTypeSupplier,
            @Nullable PageZoomIndicatorCoordinator pageZoomIndicatorCoordinator,
            NavigationAttachmentsCoordinator navigationAttachmentsCoordinator,
            @Nullable MultiInstanceManager multiInstanceManager) {
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
                omniboxUma,
                isToolbarMicEnabledSupplier,
                dropdownEmbedder,
                tabModelSelectorSupplier,
                browserControlsStateProvider,
                modalDialogManagerSupplier,
                autocompleteRequestTypeSupplier,
                pageZoomIndicatorCoordinator,
                navigationAttachmentsCoordinator,
                multiInstanceManager);
    }

    public static Class<OmniboxUma> getOmniboxUmaClass() {
        return OmniboxUma.class;
    }

    public static Class<LensController> getLensControllerClass() {
        return LensController.class;
    }

    public static Class<LocaleManager> getLocaleManagerClass() {
        return LocaleManager.class;
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

    @Override
    public void onResumeWithNative() {
        if (mTemplateUrlServiceSupplier.get() != null
                && !mTemplateUrlServiceSupplier.get().isLoaded()) {
            mTemplateUrlServiceSupplier
                    .get()
                    .runWhenLoaded(
                            () -> {
                                super.onResumeWithNative();
                            });
            return;
        }
        super.onResumeWithNative();
    }

    void updateQRButtonColors() {
        if (mLocationBarLayout instanceof BraveLocationBarLayout) {
            ((BraveLocationBarLayout) mLocationBarLayout)
                    .setQRButtonTint(
                            ThemeUtils.getThemedToolbarIconTint(mContext, BRANDED_COLOR_SCHEME));
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
            return !isUrlBarFocusedWithUserInput()
                    && (mUrlHasFocus
                            || mIsUrlFocusChangeInProgress
                            || mIsLocationBarFocusedFromNtpScroll);
        }
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
