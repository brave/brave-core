/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.status;

import android.content.Context;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.merchant_viewer.MerchantTrustSignalsCoordinator;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.permissions.PermissionDialogController;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveStatusMediator extends StatusMediator {
    // To delete in bytecode, members from parent class will be used instead.
    private boolean mUrlHasFocus;

    public BraveStatusMediator(
            PropertyModel model,
            Context context,
            UrlBarEditingTextStateProvider urlBarEditingTextStateProvider,
            boolean isTablet,
            LocationBarDataProvider locationBarDataProvider,
            PermissionDialogController permissionDialogController,
            OneshotSupplier<TemplateUrlService> templateUrlServiceSupplier,
            Supplier<Profile> profileSupplier,
            PageInfoIphController pageInfoIphController,
            WindowAndroid windowAndroid,
            @Nullable
                    Supplier<MerchantTrustSignalsCoordinator>
                            merchantTrustSignalsCoordinatorSupplier) {
        super(
                model,
                context,
                urlBarEditingTextStateProvider,
                isTablet,
                locationBarDataProvider,
                permissionDialogController,
                templateUrlServiceSupplier,
                profileSupplier,
                pageInfoIphController,
                windowAndroid,
                merchantTrustSignalsCoordinatorSupplier);
    }

    @Override
    boolean shouldDisplaySearchEngineIcon() {
        return super.shouldDisplaySearchEngineIcon() && mUrlHasFocus;
    }
}
