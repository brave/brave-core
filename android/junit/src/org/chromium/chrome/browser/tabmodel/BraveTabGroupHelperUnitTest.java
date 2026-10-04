/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabmodel;

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.Token;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.app.tabwindow.TabWindowManagerSingleton;
import org.chromium.chrome.browser.tabwindow.TabWindowManager;

import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.Set;

/** Unit tests for {@link BraveTabGroupHelper}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveTabGroupHelperUnitTest {
    private static final Token TAB_GROUP_ID_1 = new Token(1L, 2L);
    private static final Token TAB_GROUP_ID_2 = new Token(3L, 4L);
    private static final Token TAB_GROUP_ID_3 = new Token(5L, 6L);

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabWindowManager mTabWindowManager;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private TabModelSelector mOtherWindowTabModelSelector;
    @Mock private TabModel mRegularTabModel;
    @Mock private TabModel mIncognitoTabModel;
    @Mock private TabModel mOtherWindowTabModel;
    @Mock private TabUngrouper mRegularTabUngrouper;
    @Mock private TabUngrouper mIncognitoTabUngrouper;
    @Mock private TabUngrouper mOtherWindowTabUngrouper;

    @Before
    public void setUp() {
        TabWindowManagerSingleton.setTabWindowManagerForTesting(mTabWindowManager);
    }

    @Test
    public void testUngroupsEveryGroupOfEveryModelAndWindow() {
        when(mTabWindowManager.getAllTabModelSelectors())
                .thenReturn(Arrays.asList(mTabModelSelector, mOtherWindowTabModelSelector));
        when(mTabModelSelector.getModels())
                .thenReturn(Arrays.asList(mRegularTabModel, mIncognitoTabModel));
        when(mOtherWindowTabModelSelector.getModels())
                .thenReturn(Collections.singletonList(mOtherWindowTabModel));
        when(mRegularTabModel.getAllTabGroupIds()).thenReturn(tabGroupIds(TAB_GROUP_ID_1));
        when(mRegularTabModel.getTabUngrouper()).thenReturn(mRegularTabUngrouper);
        when(mIncognitoTabModel.getAllTabGroupIds()).thenReturn(tabGroupIds(TAB_GROUP_ID_2));
        when(mIncognitoTabModel.getTabUngrouper()).thenReturn(mIncognitoTabUngrouper);
        when(mOtherWindowTabModel.getAllTabGroupIds()).thenReturn(tabGroupIds(TAB_GROUP_ID_3));
        when(mOtherWindowTabModel.getTabUngrouper()).thenReturn(mOtherWindowTabUngrouper);

        BraveTabGroupHelper.ungroupAllTabGroups();

        // The user already confirmed in settings, so no per group dialog is allowed.
        verify(mRegularTabUngrouper)
                .ungroupTabGroup(TAB_GROUP_ID_1, /* trailing= */ true, /* allowDialog= */ false);
        verify(mIncognitoTabUngrouper)
                .ungroupTabGroup(TAB_GROUP_ID_2, /* trailing= */ true, /* allowDialog= */ false);
        verify(mOtherWindowTabUngrouper)
                .ungroupTabGroup(TAB_GROUP_ID_3, /* trailing= */ true, /* allowDialog= */ false);
        verifyNoMoreInteractions(
                mRegularTabUngrouper, mIncognitoTabUngrouper, mOtherWindowTabUngrouper);
    }

    @Test
    public void testUngroupsAllGroupsAndSkipsModelsWithoutGroups() {
        when(mTabWindowManager.getAllTabModelSelectors())
                .thenReturn(Collections.singletonList(mTabModelSelector));
        when(mTabModelSelector.getModels())
                .thenReturn(Arrays.asList(mRegularTabModel, mIncognitoTabModel));
        when(mRegularTabModel.getAllTabGroupIds())
                .thenReturn(tabGroupIds(TAB_GROUP_ID_1, TAB_GROUP_ID_2));
        when(mRegularTabModel.getTabUngrouper()).thenReturn(mRegularTabUngrouper);
        when(mIncognitoTabModel.getAllTabGroupIds()).thenReturn(Collections.emptySet());

        BraveTabGroupHelper.ungroupAllTabGroups();

        verify(mRegularTabUngrouper)
                .ungroupTabGroup(TAB_GROUP_ID_1, /* trailing= */ true, /* allowDialog= */ false);
        verify(mRegularTabUngrouper)
                .ungroupTabGroup(TAB_GROUP_ID_2, /* trailing= */ true, /* allowDialog= */ false);
        verifyNoMoreInteractions(mRegularTabUngrouper);
        verifyNoInteractions(mIncognitoTabUngrouper);
    }

    @Test
    public void testDoesNothingWithoutLoadedWindows() {
        when(mTabWindowManager.getAllTabModelSelectors()).thenReturn(Collections.emptyList());

        BraveTabGroupHelper.ungroupAllTabGroups();

        verifyNoInteractions(mRegularTabUngrouper);
    }

    private static Set<Token> tabGroupIds(Token... tabGroupIds) {
        return new LinkedHashSet<>(Arrays.asList(tabGroupIds));
    }
}
