/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveClassAdapter {
    public static ClassVisitor createAdapter(ClassVisitor chain) {
        chain = new BraveMainPreferenceBaseClassAdapter(chain);
        chain = new BraveAndroidSyncSettingsClassAdapter(chain);
        chain = new BraveLaunchIntentDispatcherClassAdapter(chain);
        chain = new BraveHomepageManagerClassAdapter(chain);
        chain = new BraveManageSyncSettingsClassAdapter(chain);
        chain = new BraveNewTabPageLayoutClassAdapter(chain);
        chain = new BraveFeedSurfaceCoordinatorClassAdapter(chain);
        chain = new BraveNewTabPageClassAdapter(chain);
        chain = new BraveEditUrlSuggestionProcessorClassAdapter(chain);
        chain = new BraveSearchEngineAdapterClassAdapter(chain);
        chain = new BraveCommandLineInitUtilClassAdapter(chain);
        chain = new BraveAppMenuClassAdapter(chain);
        chain = new BraveBottomControlsCoordinatorClassAdapter(chain);
        chain = new BraveToolbarManagerClassAdapter(chain);
        chain = new BraveTopToolbarCoordinatorClassAdapter(chain);
        chain = new BraveTabSwitcherModeTTCoordinatorPhoneClassAdapter(chain);
        chain = new BraveTabSwitcherModeTTPhoneClassAdapter(chain);
        chain = new BraveToolbarLayoutClassAdapter(chain);
        chain = new BraveTileViewClassAdapter(chain);
        chain = new BraveMimeUtilsClassAdapter(chain);
        chain = new BraveActivityClassAdapter(chain);
        return chain;
    }
}
