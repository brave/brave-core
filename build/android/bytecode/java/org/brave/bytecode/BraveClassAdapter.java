/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveClassAdapter {
    public static ClassVisitor createAdapter(ClassVisitor chain) {
        chain = new BraveActivityClassAdapter(chain);
        chain = new BraveAppMenuClassAdapter(chain);
        chain = new BraveAutocompleteMediatorClassAdapter(chain);
        chain = new BraveAutofillPopupBridgeClassAdapter(chain);
        chain = new BraveBookmarkUtilsClassAdapter(chain);
        chain = new BraveBottomControlsCoordinatorClassAdapter(chain);
        chain = new BraveBottomControlsMediatorClassAdapter(chain);
        chain = new BraveCommandLineInitUtilClassAdapter(chain);
        chain = new BraveContentSettingsResourcesClassAdapter(chain);
        chain = new BraveDefaultBrowserPromoUtilsClassAdapter(chain);
        chain = new BraveEditUrlSuggestionProcessorClassAdapter(chain);
        chain = new BraveFeedSurfaceCoordinatorClassAdapter(chain);
        chain = new BraveFeedSurfaceMediatorClassAdapter(chain);
        chain = new BraveFourStateCookieSettingsPreferenceBaseClassAdapter(chain);
        chain = new BraveFreIntentCreatorClassAdapter(chain);
        chain = new BraveHomepageManagerClassAdapter(chain);
        chain = new BraveIncognitoToggleTabLayoutClassAdapter(chain);
        chain = new BraveIntentHandlerClassAdapter(chain);
        chain = new BraveLaunchIntentDispatcherClassAdapter(chain);
        chain = new BraveMainPreferenceBaseClassAdapter(chain);
        chain = new BraveManageSyncSettingsClassAdapter(chain);
        chain = new BraveMenuButtonCoordinatorClassAdapter(chain);
        chain = new BraveMimeUtilsClassAdapter(chain);
        chain = new BraveNewTabPageClassAdapter(chain);
        chain = new BraveNewTabPageLayoutClassAdapter(chain);
        chain = new BraveNotificationManagerProxyImplClassAdapter(chain);
        chain = new BravePasswordSettingsBaseClassAdapter(chain);
        chain = new BravePermissionDialogDelegateClassAdapter(chain);
        chain = new BravePermissionDialogModelClassAdapter(chain);
        chain = new BraveQueryTileSectionClassAdapter(chain);
        chain = new BraveSearchEngineAdapterClassAdapter(chain);
        chain = new BraveSettingsLauncherImplClassAdapter(chain);
        chain = new BraveShareDelegateImplClassAdapter(chain);
        chain = new BraveSingleCategorySettingsClassAdapter(chain);
        chain = new BraveSingleWebsiteSettingsClassAdapter(chain);
        chain = new BraveSiteSettingsCategoryClassAdapter(chain);
        chain = new BraveSiteSettingsDelegateClassAdapter(chain);
        chain = new BraveSiteSettingsPreferencesBaseClassAdapter(chain);
        chain = new BraveStatusMediatorClassAdapter(chain);
        chain = new BraveTabGroupUiCoordinatorClassAdapter(chain);
        chain = new BraveTabSwitcherModeTTCoordinatorClassAdapter(chain);
        chain = new BraveTabSwitcherModeTopToolbarClassAdapter(chain);
        chain = new BraveTabUiThemeProviderClassAdapter(chain);
        chain = new BraveTabbedActivityClassAdapter(chain);
        chain = new BraveThemeUtilsClassAdapter(chain);
        chain = new BraveTileViewClassAdapter(chain);
        chain = new BraveToolbarLayoutClassAdapter(chain);
        chain = new BraveToolbarManagerClassAdapter(chain);
        chain = new BraveTopToolbarCoordinatorClassAdapter(chain);
        chain = new BraveVariationsSeedFetcherClassAdapter(chain);
        chain = new BraveWebsiteClassAdapter(chain);
        chain = new BraveWebsitePermissionsFetcherClassAdapter(chain);
        return chain;
    }
}
