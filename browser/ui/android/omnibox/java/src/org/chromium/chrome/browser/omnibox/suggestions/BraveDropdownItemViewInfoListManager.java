/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

class BraveDropdownItemViewInfoListManager extends DropdownItemViewInfoListManager {
    private final ModelList mManagedModel;

    BraveDropdownItemViewInfoListManager(
            @NonNull ModelList managedModel, @NonNull Context context) {
        super(managedModel, context);

        mManagedModel = managedModel;
    }

    /**
     * Removes Brave search suggestion.
     */
    public void removeBraveSearchSuggestion() {
        int index;
        int count = 0;

        for (index = mManagedModel.size() - 1; index >= 0; index--) {
            DropdownItemViewInfo viewInfo = (DropdownItemViewInfo) mManagedModel.get(index);
            if (viewInfo.processor.getViewTypeId()
                    == BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER) {
                count++;
            } else if (count > 0
                    && viewInfo.processor.getViewTypeId()
                            != BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER) {
                break;
            }
        }
        if (count > 0) {
            // Skip group header when dropping items.
            mManagedModel.removeRange(index + 1, count);
        }
    }
}
