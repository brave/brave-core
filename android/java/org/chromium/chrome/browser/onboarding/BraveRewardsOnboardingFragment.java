/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.method.ScrollingMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.util.BraveTouchUtils;

public class BraveRewardsOnboardingFragment extends Fragment {
    private OnViewPagerAction onViewPagerAction;

    private TextView tvText;
    private TextView tvAgree;

    private Button btnSkip;
    private Button btnNext;

    public BraveRewardsOnboardingFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        // Inflate the layout for this fragment
        View root = inflater.inflate(R.layout.fragment_brave_rewards_onboarding, container, false);

        initializeViews(root);

        setActions();

        return root;
    }

    private void initializeViews(View root) {
        tvText = root.findViewById(R.id.section_text);

        tvAgree = root.findViewById(R.id.agree_text);

        btnSkip = root.findViewById(R.id.btn_skip);
        btnNext = root.findViewById(R.id.btn_next);

        BraveTouchUtils.ensureMinTouchTarget(btnSkip);
    }

    private void setActions() {
        btnNext.setText(getResources().getString(R.string.earn_and_give));
        btnSkip.setText(getResources().getString(R.string.skip));

        String braveRewardsText = "<b>"
                + String.format(getResources().getString(R.string.earn_tokens),
                        getResources().getString(R.string.token))
                + "</b> " + getResources().getString(R.string.brave_rewards_onboarding_text);
        Spanned textToInsert = BraveRewardsHelper.spannedFromHtmlString(braveRewardsText);
        tvText.setText(textToInsert);
        tvText.setMovementMethod(new ScrollingMovementMethod());

        String termsText = getResources().getString(R.string.terms_text) + " "
                           + getResources().getString(R.string.terms_of_service) + ".";
        Spanned textToAgree = BraveRewardsHelper.spannedFromHtmlString(termsText);
        SpannableString ss = new SpannableString(textToAgree.toString());

        ClickableSpan clickableSpan =
                new ClickableSpan() {
                    @Override
                    public void onClick(@NonNull View textView) {
                        CustomTabActivity.showInfoPage(
                                getActivity(), BraveActivity.BRAVE_TERMS_PAGE);
                    }

                    @Override
                    public void updateDrawState(@NonNull TextPaint ds) {
                        super.updateDrawState(ds);
                        ds.setUnderlineText(false);
                    }
                };

        ss.setSpan(clickableSpan, getResources().getString(R.string.terms_text).length(),
                   ss.length() - 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        ForegroundColorSpan foregroundSpan =
                new ForegroundColorSpan(getContext().getColor(R.color.onboarding_orange));
        ss.setSpan(foregroundSpan, getResources().getString(R.string.terms_text).length(),
                   ss.length() - 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tvAgree.setMovementMethod(LinkMovementMethod.getInstance());
        tvAgree.setText(ss);

        btnSkip.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                assert onViewPagerAction != null;
                if (onViewPagerAction != null)
                    onViewPagerAction.onSkip();
            }
        });

        btnNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                assert onViewPagerAction != null;
                if (onViewPagerAction != null)
                    onViewPagerAction.onNext();
            }
        });
    }

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        this.onViewPagerAction = onViewPagerAction;
    }
}
