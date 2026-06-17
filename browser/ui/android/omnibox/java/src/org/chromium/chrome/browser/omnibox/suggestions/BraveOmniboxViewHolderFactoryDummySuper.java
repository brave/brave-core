/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import org.chromium.build.annotations.NullMarked;
import org.chromium.ui.modelutil.MVCListAdapter.ViewBuilder;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor.ViewBinder;

/**
 * Holds same-named stubs that bytecode-redirect to private members of {@link
 * OmniboxViewHolderFactory}, so {@link BraveOmniboxViewHolderFactory} can call registerType()
 * despite Java visibility rules. The stub bodies are never executed: {@code
 * BraveOmniboxViewHolderFactoryClassAdapter} rewrites each call site to invoke the upstream method
 * (which it also bumps to public at bytecode time).
 */
@NullMarked
class BraveOmniboxViewHolderFactoryDummySuper extends OmniboxViewHolderFactory {
    BraveOmniboxViewHolderFactoryDummySuper() {
        super();
        assert false : "This class usage should be removed in the bytecode!";
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    <T extends android.view.View> void registerType(
            int typeId, ViewBuilder<T> builder, ViewBinder<PropertyModel, T, PropertyKey> binder) {
        assert false : "This class usage should be removed in the bytecode!";
    }
}
