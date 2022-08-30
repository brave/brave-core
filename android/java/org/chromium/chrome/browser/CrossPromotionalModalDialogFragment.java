/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;

public class CrossPromotionalModalDialogFragment
        extends BraveDialogFragment implements View.OnClickListener {
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_cross_promotional_modal_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        Button mDoneButton = view.findViewById(R.id.btn_done);
        mDoneButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        dismiss();
    }
}
