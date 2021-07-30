/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.status;

import android.content.Context;
import android.content.res.Resources;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.SearchEngineLogoUtils;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.permissions.PermissionDialogController;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveStatusMediator extends StatusMediator {
    // To delete in bytecode, members from parent class will be used instead.
    private boolean mUrlHasFocus;

    public BraveStatusMediator(PropertyModel model, Resources resources, Context context,
            UrlBarEditingTextStateProvider urlBarEditingTextStateProvider, boolean isTablet,
            LocationBarDataProvider locationBarDataProvider,
            PermissionDialogController permissionDialogController,
            SearchEngineLogoUtils searchEngineLogoUtils,
            OneshotSupplier<TemplateUrlService> templateUrlServiceSupplier,
            Supplier<Profile> profileSupplier, PageInfoIPHController pageInfoIPHController,
            WindowAndroid windowAndroid) {
        super(model, resources, context, urlBarEditingTextStateProvider, isTablet,
                locationBarDataProvider, permissionDialogController, searchEngineLogoUtils,
                templateUrlServiceSupplier, profileSupplier, pageInfoIPHController, windowAndroid);
    }

    @Override
    boolean shouldDisplaySearchEngineIcon() {
        return super.shouldDisplaySearchEngineIcon() && mUrlHasFocus;
    }
}
