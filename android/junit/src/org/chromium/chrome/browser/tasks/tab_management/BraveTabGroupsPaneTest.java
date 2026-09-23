/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.LazyOneshotSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.hub.PaneHubController;
import org.chromium.chrome.browser.hub.PaneId;
import org.chromium.chrome.browser.hub.PaneManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.components.tab_group_sync.TabGroupUiActionHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link BraveTabGroupsPane}. Verifies that the pane offers its Hub button only
 * while the "Enable tab groups" master switch is on, and that toggling the switch applies to an
 * already constructed pane, i.e. without a browser restart.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveTabGroupsPaneTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabModel mTabModel;
    @Mock private OneshotSupplier<ProfileProvider> mProfileProviderSupplier;
    @Mock private PaneManager mPaneManager;
    @Mock private TabGroupUiActionHandler mTabGroupUiActionHandler;
    @Mock private DataSharingTabManager mDataSharingTabManager;
    @Mock private PaneHubController mPaneHubController;

    private final List<BraveTabGroupsPane> mPanes = new ArrayList<>();

    @After
    public void tearDown() {
        for (BraveTabGroupsPane pane : mPanes) {
            pane.destroy();
        }
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testReferenceButton_tabGroupsEnabled_paneIsOffered() {
        setTabGroupsEnabled(true);

        assertNotNull(createPane().getReferenceButtonDataSupplier().get());
    }

    @Test
    public void testReferenceButton_tabGroupsDisabled_paneIsNotOffered() {
        setTabGroupsEnabled(false);

        // Without reference button data the Hub shows no pane switcher button and refuses to focus
        // the pane.
        assertNull(createPane().getReferenceButtonDataSupplier().get());
    }

    @Test
    public void testReferenceButton_switchToggledAfterConstruction_appliesWithoutRestart() {
        setTabGroupsEnabled(true);
        BraveTabGroupsPane pane = createPane();

        // The Hub builds its pane list once per activity, so the pane outlives the switch and has
        // to follow it. Without this the change would only show up after a browser restart.
        setTabGroupsEnabled(false);
        assertNull(pane.getReferenceButtonDataSupplier().get());

        setTabGroupsEnabled(true);
        assertNotNull(pane.getReferenceButtonDataSupplier().get());
    }

    @Test
    public void testSwitchTurnedOff_whileFocused_leavesPane() {
        setTabGroupsEnabled(true);
        BraveTabGroupsPane pane = createPane();
        // The Hub only hands out a controller to the focused pane.
        pane.setPaneHubController(mPaneHubController);

        setTabGroupsEnabled(false);

        verify(mPaneHubController).focusPane(PaneId.TAB_SWITCHER);
    }

    @Test
    public void testSwitchTurnedOff_whileUnfocused_staysPut() {
        setTabGroupsEnabled(true);
        BraveTabGroupsPane pane = createPane();
        pane.setPaneHubController(mPaneHubController);
        pane.setPaneHubController(null);

        setTabGroupsEnabled(false);

        verify(mPaneHubController, never()).focusPane(PaneId.TAB_SWITCHER);
    }

    @Test
    public void testDestroy_stopsFollowingTheSwitch() {
        setTabGroupsEnabled(false);
        BraveTabGroupsPane pane = createPane();

        pane.destroy();
        setTabGroupsEnabled(true);

        assertNull(pane.getReferenceButtonDataSupplier().get());
    }

    private static void setTabGroupsEnabled(boolean enabled) {
        BraveTabUiFeatureUtilities.setTabGroupsEnabled(enabled);
    }

    private BraveTabGroupsPane createPane() {
        BraveTabGroupsPane pane =
                new BraveTabGroupsPane(
                        ContextUtils.getApplicationContext(),
                        LazyOneshotSupplier.fromSupplier(() -> mTabModel),
                        /* onToolbarAlphaChange= */ alpha -> {},
                        mProfileProviderSupplier,
                        () -> mPaneManager,
                        () -> mTabGroupUiActionHandler,
                        /* modalDialogManagerSupplier= */ () -> null,
                        ObservableSuppliers.createMonotonic(),
                        mDataSharingTabManager);
        mPanes.add(pane);
        return pane;
    }
}
