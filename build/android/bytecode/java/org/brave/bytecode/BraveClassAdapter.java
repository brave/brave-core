/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveClassAdapter {
    public static ClassVisitor createAdapter(ClassVisitor chain) {
        chain = new BraveActivityClassAdapter(chain);
        chain = new BraveAdsNotificationBuilderClassAdapter(chain);
        chain = new BraveAppHooksClassAdapter(chain);
        chain = new BraveAppMenuClassAdapter(chain);
        chain = new BraveApplicationImplBaseClassAdapter(chain);
        chain = new BraveAutocompleteCoordinatorClassAdapter(chain);
        chain = new BraveAutocompleteMediatorClassAdapter(chain);
        chain = new BraveAutofillPopupBridgeClassAdapter(chain);
        chain = new BraveBookmarkUtilsClassAdapter(chain);
        chain = new BraveBottomControlsCoordinatorClassAdapter(chain);
        chain = new BraveBottomControlsMediatorClassAdapter(chain);
        chain = new BraveCachedFlagClassAdapter(chain);
        chain = new BraveCommandLineInitUtilClassAdapter(chain);
        chain = new BraveContentSettingsResourcesClassAdapter(chain);
        chain = new BraveCustomizationProviderDelegateImplClassAdapter(chain);
        chain = new BraveDefaultBrowserPromoUtilsClassAdapter(chain);
        chain = new BraveDownloadMessageUiControllerImplClassAdapter(chain);
        chain = new BraveDropdownItemViewInfoListBuilderClassAdapter(chain);
        chain = new BraveDropdownItemViewInfoListManagerClassAdapter(chain);
        chain = new BraveDynamicColorsClassAdapter(chain);
        chain = new BraveEditUrlSuggestionProcessorClassAdapter(chain);
        chain = new BraveFeedSurfaceCoordinatorClassAdapter(chain);
        chain = new BraveFeedSurfaceMediatorClassAdapter(chain);
        chain = new BraveFourStateCookieSettingsPreferenceBaseClassAdapter(chain);
        chain = new BraveFreIntentCreatorClassAdapter(chain);
        chain = new BraveHomepageManagerClassAdapter(chain);
        chain = new BraveIncognitoToggleTabLayoutClassAdapter(chain);
        chain = new BraveIntentHandlerClassAdapter(chain);
        chain = new BraveLauncherActivityClassAdapter(chain);
        chain = new BraveLaunchIntentDispatcherClassAdapter(chain);
        chain = new BraveLocationBarCoordinatorClassAdapter(chain);
        chain = new BraveLocationBarLayoutClassAdapter(chain);
        chain = new BraveLocationBarMediatorClassAdapter(chain);
        chain = new BraveLogoMediatorClassAdapter(chain);
        chain = new BraveMainPreferenceBaseClassAdapter(chain);
        chain = new BraveManageAccountDevicesLinkViewClassAdapter(chain);
        chain = new BraveManageSyncSettingsClassAdapter(chain);
        chain = new BraveTranslateCompactInfoBarBaseClassAdapter(chain);
        chain = new BraveMenuButtonCoordinatorClassAdapter(chain);
        chain = new BraveMimeUtilsClassAdapter(chain);
        chain = new BraveMostVisitedTilesMediatorClassAdapter(chain);
        chain = new BraveNewTabPageClassAdapter(chain);
        chain = new BraveNewTabPageLayoutClassAdapter(chain);
        chain = new BraveNotificationManagerProxyImplClassAdapter(chain);
        chain = new BraveNotificationPermissionRationaleDialogControllerClassAdapter(chain);
        chain = new BravePasswordSettingsBaseClassAdapter(chain);
        chain = new BravePermissionDialogDelegateClassAdapter(chain);
        chain = new BravePermissionDialogModelClassAdapter(chain);
        chain = new BravePreferenceFragmentClassAdapter(chain);
        chain = new BravePureJavaExceptionReporterClassAdapter(chain);
        chain = new BraveQueryTileSectionClassAdapter(chain);
        chain = new BraveReaderModeManagerClassAdapter(chain);
        chain = new BraveReturnToChromeUtilClassAdapter(chain);
        chain = new BraveSearchEngineAdapterClassAdapter(chain);
        chain = new BraveSettingsLauncherImplClassAdapter(chain);
        chain = new BraveShareDelegateImplClassAdapter(chain);
        chain = new BraveSingleCategorySettingsClassAdapter(chain);
        chain = new BraveSingleWebsiteSettingsClassAdapter(chain);
        chain = new BraveSiteSettingsCategoryClassAdapter(chain);
        chain = new BraveSiteSettingsDelegateClassAdapter(chain);
        chain = new BraveSiteSettingsPreferencesBaseClassAdapter(chain);
        chain = new BraveStartupPaintPreviewHelperClassAdapter(chain);
        chain = new BraveStatusMediatorClassAdapter(chain);
        chain = new BraveTabGroupUiCoordinatorClassAdapter(chain);
        chain = new BraveTabSwitcherModeTTCoordinatorClassAdapter(chain);
        chain = new BraveTabSwitcherModeTopToolbarClassAdapter(chain);
        chain = new BraveTabUiThemeProviderClassAdapter(chain);
        chain = new BraveTabbedActivityClassAdapter(chain);
        chain = new BraveTemplateUrlServiceFactoryClassAdapter(chain);
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
