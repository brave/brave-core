/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabmodel;

import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabHidingType;

public class BraveTabModelSelectorTabObserver extends TabModelSelectorTabObserver {
   private boolean mTabHiddenByChangedTabs;
   /**
    * Constructs an observer that should be notified of tab changes for all tabs owned
    * by a specified {@link TabModelSelector}.  Any Tabs created after this call will be
    * observed as well, and Tabs removed will no longer have their information broadcast.
    *
    * <p>
    * {@link #destroy()} must be called to unregister this observer.
    *
    * @param selector The selector that owns the Tabs that should notify this observer.
    */
   public BraveTabModelSelectorTabObserver(TabModelSelector selector) {
       super(selector);

   }

   @Override
   public void onHidden(Tab tab, int reason) {
      mTabHiddenByChangedTabs = reason == TabHidingType.CHANGED_TABS;
      super.onHidden(tab, reason);
   }

   public boolean isTabHiddenByChangedTabs() {
      return mTabHiddenByChangedTabs;
   }
}
