/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.contextmenu;

import android.content.Context;

import androidx.browser.customtabs.CustomContentAction;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.shields.UrlSanitizerServiceFactory;
import org.chromium.chrome.browser.tab.TabContextMenuItemDelegate;
import org.chromium.components.embedder_support.contextmenu.ContextMenuItemDelegate;
import org.chromium.components.embedder_support.contextmenu.ContextMenuNativeDelegate;
import org.chromium.components.embedder_support.contextmenu.ContextMenuParams;
import org.chromium.url_sanitizer.mojom.UrlSanitizerService;

import java.util.List;

public class BraveChromeContextMenuPopulator extends ChromeContextMenuPopulator {
    // To be deleted via bytecode and super field to be used
    private TabContextMenuItemDelegate mItemDelegate;
    // To be deleted via bytecode and super field to be used
    private ContextMenuParams mParams;

    public BraveChromeContextMenuPopulator(
            TabContextMenuItemDelegate itemDelegate,
            Supplier<ShareDelegate> shareDelegate,
            List<CustomContentAction> customContentActions,
            @ContextMenuMode int mode,
            Context context,
            ContextMenuParams params,
            ContextMenuNativeDelegate nativeDelegate) {
        super(
                itemDelegate,
                shareDelegate,
                customContentActions,
                mode,
                context,
                params,
                nativeDelegate);
    }

    @Override
    public boolean onItemSelected(int itemId) {
        if (itemId != R.id.contextmenu_copy_clean_link) {
            return super.onItemSelected(itemId);
        }
        UrlSanitizerService urlSanitizerService =
                UrlSanitizerServiceFactory.getInstance()
                        .getUrlSanitizerAndroidService(getProfile(), null);
        if (urlSanitizerService != null) {
            urlSanitizerService.sanitizeUrl(
                    mParams.getUnfilteredLinkUrl().getSpec(),
                    result -> {
                        mItemDelegate.onSaveToClipboard(
                                result, ContextMenuItemDelegate.ClipboardType.LINK_URL);
                        urlSanitizerService.close();
                    });
        }

        return true;
    }

    private Profile getProfile() {
        assert false : "This method should be overridden via bytecode manipulation!";
        return null;
    }
}
