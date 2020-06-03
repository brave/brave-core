/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import androidx.fragment.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.onboarding.OnViewPagerAction;

public class TroubleshootingOnboardingFragment extends Fragment {
    private OnViewPagerAction onViewPagerAction;

    private Button btnContinueToWallet;

    public TroubleshootingOnboardingFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View root =
                inflater.inflate(R.layout.fragment_troubleshooting_onboarding, container, false);

        initializeViews(root);

        setActions();

        return root;
    }

    private void setActions() {
        btnContinueToWallet.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                assert onViewPagerAction != null;
                if (onViewPagerAction != null) onViewPagerAction.onContinueToWallet();
            }
        });
    }

    private void initializeViews(View root) {
        btnContinueToWallet = root.findViewById(R.id.btn_continue_to_wallet);
    }

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        this.onViewPagerAction = onViewPagerAction;
    }
}
