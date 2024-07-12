/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

/** Adapter to perform Java asm patches on upstreams' classes. */
public class BraveClassAdapter {
    public static ClassVisitor createAdapter(ClassVisitor chain) {
        chain = new BraveActivityClassAdapter(chain);
        chain = new BraveAppHooksClassAdapter(chain);
        chain = new BraveAppMenuClassAdapter(chain);
        chain = new BraveApplicationImplBaseClassAdapter(chain);
        chain = new BraveAutocompleteCoordinatorClassAdapter(chain);
        chain = new BraveAutocompleteEditTextClassAdapter(chain);
        chain = new BraveAutocompleteMediatorBaseClassAdapter(chain);
        chain = new BraveAutocompleteMediatorClassAdapter(chain);
        chain = new BraveBookmarkActivityClassAdapter(chain);
        chain = new BraveBookmarkBridgeClassAdapter(chain);
        chain = new BraveBookmarkDelegateClassAdapter(chain);
        chain = new BraveBookmarkManagerCoordinatorClassAdapter(chain);
        chain = new BraveBookmarkManagerMediatorClassAdapter(chain);
        chain = new BraveBookmarkModelClassAdapter(chain);
        chain = new BraveBookmarkPageClassAdapter(chain);
        chain = new BraveBookmarkToolbarClassAdapter(chain);
        chain = new BraveBookmarkToolbarCoordinatorClassAdapter(chain);
        chain = new BraveBookmarkUiPrefsClassAdapter(chain);
        chain = new BraveBookmarkUtilsClassAdapter(chain);
        chain = new BraveBottomControlsCoordinatorClassAdapter(chain);
        chain = new BraveBottomControlsMediatorClassAdapter(chain);
        chain = new BraveCachedFlagClassAdapter(chain);
        chain = new BraveChromeContextMenuPopulatorAdapter(chain);
        chain = new BraveCommandLineInitUtilClassAdapter(chain);
        chain = new BraveContentSettingsResourcesClassAdapter(chain);
        chain = new BraveContentViewClassAdapter(chain);
        chain = new BraveCustomizationProviderDelegateImplClassAdapter(chain);
        chain = new BraveDefaultBrowserPromoUtilsClassAdapter(chain);
        chain = new BraveDownloadMessageUiControllerImplClassAdapter(chain);
        chain = new BraveDropdownItemViewInfoListBuilderClassAdapter(chain);
        chain = new BraveDropdownItemViewInfoListManagerClassAdapter(chain);
        chain = new BraveDynamicColorsClassAdapter(chain);
        chain = new BraveEditUrlSuggestionProcessorBaseClassAdapter(chain);
        chain = new BraveEditUrlSuggestionProcessorClassAdapter(chain);
        chain = new BraveExportFlowClassAdapter(chain);
        chain = new BraveExternalNavigationHandlerClassAdapter(chain);
        chain = new BraveFeedSurfaceCoordinatorClassAdapter(chain);
        chain = new BraveFeedSurfaceMediatorClassAdapter(chain);
        chain = new BraveFreIntentCreatorClassAdapter(chain);
        chain = new BraveHelpAndFeedbackLauncherImplClassAdapter(chain);
        chain = new BraveHomepageManagerClassAdapter(chain);
        chain = new BraveHubManagerImplClassAdapter(chain);
        chain = new BraveIdentityDiscControllerClassAdapter(chain);
        chain = new BraveIntentHandlerClassAdapter(chain);
        chain = new BraveLauncherActivityClassAdapter(chain);
        chain = new BraveLaunchIntentDispatcherClassAdapter(chain);
        chain = new BraveLocaleManagerDelegateImplClassAdapter(chain);
        chain = new BraveLocationBarCoordinatorClassAdapter(chain);
        chain = new BraveLocationBarLayoutClassAdapter(chain);
        chain = new BraveLocationBarMediatorClassAdapter(chain);
        chain = new BraveLogoMediatorClassAdapter(chain);
        chain = new BraveMainPreferenceBaseClassAdapter(chain);
        chain = new BraveManageAccountDevicesLinkViewClassAdapter(chain);
        chain = new BraveManageSyncSettingsClassAdapter(chain);
        chain = new BraveMultiInstanceManagerApi31ClassAdapter(chain);
        chain = new BraveTranslateCompactInfoBarBaseClassAdapter(chain);
        chain = new BraveMenuButtonCoordinatorClassAdapter(chain);
        chain = new BraveMimeUtilsClassAdapter(chain);
        chain = new BraveMostVisitedTilesLayoutBaseClassAdapter(chain);
        chain = new BraveMostVisitedTilesMediatorClassAdapter(chain);
        chain = new BraveMultiWindowUtilsClassAdapter(chain);
        chain = new BraveNewTabPageClassAdapter(chain);
        chain = new BraveNewTabPageLayoutClassAdapter(chain);
        chain = new BraveNotificationBuilderClassAdapter(chain);
        chain = new BraveNotificationManagerProxyImplClassAdapter(chain);
        chain = new BraveNotificationPermissionRationaleDialogControllerClassAdapter(chain);
        chain = new BravePartialCustomTabBottomSheetStrategyClassAdapter(chain);
        chain = new BravePasswordAccessReauthenticationHelperClassAdapter(chain);
        chain = new BravePasswordSettingsBaseClassAdapter(chain);
        chain = new BravePermissionDialogDelegateClassAdapter(chain);
        chain = new BravePermissionDialogModelClassAdapter(chain);
        chain = new BravePictureInPictureActivityClassAdapter(chain);
        chain = new BravePreferenceFragmentClassAdapter(chain);
        chain = new BravePureJavaExceptionReporterClassAdapter(chain);
        chain = new BraveReaderModeManagerClassAdapter(chain);
        chain = new BraveReturnToChromeUtilClassAdapter(chain);
        chain = new BraveSearchEngineAdapterClassAdapter(chain);
        chain = new BraveSearchEnginePreferenceClassAdapter(chain);
        chain = new BraveSettingsLauncherImplClassAdapter(chain);
        chain = new BraveShareDelegateImplClassAdapter(chain);
        chain = new BraveSingleCategorySettingsClassAdapter(chain);
        chain = new BraveSingleWebsiteSettingsClassAdapter(chain);
        chain = new BraveSiteSettingsCategoryClassAdapter(chain);
        chain = new BraveSiteSettingsDelegateClassAdapter(chain);
        chain = new BraveSiteSettingsPreferencesBaseClassAdapter(chain);
        chain = new BraveStatusMediatorClassAdapter(chain);
        chain = new BraveStrictPreferenceKeyCheckerClassAdapter(chain);
        chain = new BraveSwipeRefreshHandlerClassAdapter(chain);
        chain = new BraveSystemAccountManagerDelegateAdapter(chain);
        chain = new BraveTabGroupUiCoordinatorClassAdapter(chain);
        chain = new BraveTabHelpersClassAdapter(chain);
        chain = new BraveTabUiThemeProviderClassAdapter(chain);
        chain = new BraveTabUiThemeUtilsClassAdapter(chain);
        chain = new BraveTabbedActivityClassAdapter(chain);
        chain = new BraveTabbedRootUiCoordinatorClassAdapter(chain);
        chain = new BraveTabGroupModelFilterClassAdapter(chain);
        chain = new BraveThemeUtilsClassAdapter(chain);
        chain = new BraveTileViewClassAdapter(chain);
        chain = new BraveToolbarLayoutClassAdapter(chain);
        chain = new BraveToolbarManagerClassAdapter(chain);
        chain = new BraveToolbarSwipeLayoutClassAdapter(chain);
        chain = new BraveVariationsSeedFetcherClassAdapter(chain);
        chain = new BraveWebsiteClassAdapter(chain);
        chain = new BraveWebsitePermissionsFetcherClassAdapter(chain);
        return chain;
    }
}
