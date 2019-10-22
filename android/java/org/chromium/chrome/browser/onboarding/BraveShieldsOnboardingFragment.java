/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.Fragment;
import android.os.Bundle;
import android.text.Spanned;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.onboarding.OnViewPagerAction;

public class BraveShieldsOnboardingFragment extends Fragment {
    private TextView tvText;

    private Button btnSkip, btnNext;

    private OnViewPagerAction onViewPagerAction;

    public BraveShieldsOnboardingFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View root = inflater.inflate(R.layout.fragment_brave_shields_onboarding, container, false);

        initializeViews(root);

        setActions();

        return root;
    }

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        this.onViewPagerAction = onViewPagerAction;
    }

    private void initializeViews(View root) {
        tvText = root.findViewById(R.id.section_text);

        btnSkip = root.findViewById(R.id.btn_skip);
        btnNext = root.findViewById(R.id.btn_next);
    }

    private void setActions() {
        String braveShieldsText = "<b>" + getResources().getString(R.string.block) + "</b> "
                + getResources().getString(R.string.brave_shields_onboarding_text);
        Spanned textToInsert = BraveRewardsHelper.spannedFromHtmlString(braveShieldsText);
        tvText.setText(textToInsert);
        tvText.setMovementMethod(new ScrollingMovementMethod());

        btnSkip.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                assert onViewPagerAction != null;
                if (onViewPagerAction != null) onViewPagerAction.onSkip();
            }
        });

        btnNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                assert onViewPagerAction != null;
                if (onViewPagerAction != null) onViewPagerAction.onNext();
            }
        });
    }
}
