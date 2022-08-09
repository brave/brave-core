/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

class BraveDropdownItemViewInfoListManager extends DropdownItemViewInfoListManager {
    BraveDropdownItemViewInfoListManager(
            @NonNull ModelList managedModel, @NonNull Context context) {
        super(managedModel, context);
    }

    public void removeSuggestionsForGroup(int groupId) {
        assert false : "removeSuggestionsForGroup should be redirected to parent in bytecode!";
    }
}
