/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.components.omnibox.suggestions.OmniboxSuggestionUiType;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

class BraveDropdownItemViewInfoListManager extends DropdownItemViewInfoListManager {
    private final ModelList mManagedModel;

    BraveDropdownItemViewInfoListManager(
            @NonNull ModelList managedModel, @NonNull Context context) {
        super(managedModel, context);

        mManagedModel = managedModel;
    }

    /**
     * Remove all suggestions that belong to specific group.
     *
     * @param groupId Group ID of suggestions that should be removed.
     */
    public void removeSuggestionsForGroup(int groupId) {
        int index;
        int count = 0;

        for (index = mManagedModel.size() - 1; index >= 0; index--) {
            DropdownItemViewInfo viewInfo = (DropdownItemViewInfo) mManagedModel.get(index);
            if (isGroupHeaderWithId(viewInfo, groupId)) {
                break;
            } else if (viewInfo.groupId == groupId) {
                count++;
            } else if (count > 0 && viewInfo.groupId != groupId) {
                break;
            }
        }
        if (count > 0) {
            // Skip group header when dropping items.
            mManagedModel.removeRange(index + 1, count);
        }
    }

    /** @return Whether the supplied view info is a header for the specific group of suggestions. */
    private boolean isGroupHeaderWithId(DropdownItemViewInfo info, int groupId) {
        return (info.type == OmniboxSuggestionUiType.HEADER && info.groupId == groupId);
    }
}
