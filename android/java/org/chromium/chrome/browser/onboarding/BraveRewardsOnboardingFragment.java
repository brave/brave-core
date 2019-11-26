/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.AlarmManager;
import android.app.Fragment;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.method.ScrollingMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.onboarding.BraveRewardsServiceReceiver;
import org.chromium.chrome.browser.onboarding.OnViewPagerAction;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.PackageUtils;

public class BraveRewardsOnboardingFragment extends Fragment {
    private OnViewPagerAction onViewPagerAction;

    private ImageView bgImage;

    private TextView tvTitle, tvText, tvAgree;

    private Button btnSkip, btnNext;

    private static final String BRAVE_TERMS_PAGE =
            "https://basicattentiontoken.org/user-terms-of-service/";

    private int onboardingType = OnboardingPrefManager.NEW_USER_ONBOARDING;

    private boolean fromSettings;

    private boolean isAdsAvailable;

    public BraveRewardsOnboardingFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        isAdsAvailable = OnboardingPrefManager.getInstance().isAdsAvailable();

        // Inflate the layout for this fragment
        View root = inflater.inflate(R.layout.fragment_brave_rewards_onboarding, container, false);

        initializeViews(root);

        setActions();

        return root;
    }

    private void initializeViews(View root) {
        bgImage = root.findViewById(R.id.bg_image);

        tvTitle = root.findViewById(R.id.section_title);
        tvText = root.findViewById(R.id.section_text);

        tvAgree = root.findViewById(R.id.agree_text);

        btnSkip = root.findViewById(R.id.btn_skip);
        btnNext = root.findViewById(R.id.btn_next);
    }

    private void setActions() {
        if (fromSettings) {
            if (!isAdsAvailable)
                btnNext.setText(getResources().getString(R.string.finish));
            else
                btnNext.setText(getResources().getString(R.string.next));
            btnSkip.setText(getResources().getString(R.string.skip));
        } else {
            btnSkip.setText(getResources().getString(R.string.no_thanks));
        }

        Spanned textToInsert;

        if (onboardingType == OnboardingPrefManager.EXISTING_USER_REWARDS_ON_ONBOARDING) {
            bgImage.setImageResource(R.drawable.android_br_on);

            tvTitle.setText(getResources().getString(R.string.brave_ads_existing_user_offer_title));

            String braveRewardsText = "<b>" + getResources().getString(R.string.earn_tokens)
                    + "</b> " + getResources().getString(R.string.brave_rewards_onboarding_text2);
            textToInsert = BraveRewardsHelper.spannedFromHtmlString(braveRewardsText);
            tvText.setText(textToInsert);

            btnNext.setText(getResources().getString(R.string.turn_on));
        } else {
            String braveRewardsText = "<b>" + getResources().getString(R.string.earn_tokens)
                    + "</b> " + getResources().getString(R.string.brave_rewards_onboarding_text);
            textToInsert = BraveRewardsHelper.spannedFromHtmlString(braveRewardsText);
            tvText.setText(textToInsert);
        }
        tvText.setMovementMethod(new ScrollingMovementMethod());

        String termsText = getResources().getString(R.string.terms_text) + " "
                + getResources().getString(R.string.terms_of_service) + ".";
        Spanned textToAgree = BraveRewardsHelper.spannedFromHtmlString(termsText);
        SpannableString ss = new SpannableString(textToAgree.toString());

        ClickableSpan clickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                CustomTabActivity.showInfoPage(getActivity(), BRAVE_TERMS_PAGE);
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
                new ForegroundColorSpan(getResources().getColor(R.color.onboarding_orange));
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
                if (fromSettings) {
                    if (!isAdsAvailable) {
                        getActivity().finish();
                    }
                    assert onViewPagerAction != null;
                    if (onViewPagerAction != null) onViewPagerAction.onNext();
                } else if (onboardingType
                        == OnboardingPrefManager.EXISTING_USER_REWARDS_ON_ONBOARDING) {
                    BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedProfile());
                    assert onViewPagerAction != null;
                    if (onViewPagerAction != null) onViewPagerAction.onNext();
                } else {
                    AlarmManager alarmManager = (AlarmManager) getActivity().getSystemService(Context.ALARM_SERVICE);
                    Intent broadcast_intent = new Intent(getActivity(), BraveRewardsServiceReceiver.class);
                    PendingIntent pendingIntent = PendingIntent.getBroadcast(getActivity(), 0,  broadcast_intent, 0);
                    alarmManager.set(AlarmManager.RTC_WAKEUP, 0, pendingIntent);

                    if (PackageUtils.isFirstInstall(getActivity()) && !isAdsAvailable) {
                        OnboardingPrefManager.getInstance().setPrefOnboardingEnabled(false);
                        getActivity().finish();
                    } else {
                        assert onViewPagerAction != null;
                        if (onViewPagerAction != null)
                            onViewPagerAction.onNext();
                    }
                }
            }
        });
    }

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        this.onViewPagerAction = onViewPagerAction;
    }

    public void setOnboardingType(int onboardingType) {
        this.onboardingType = onboardingType;
    }

    public void setFromSettings(boolean fromSettings) {
        this.fromSettings = fromSettings;
    }
}
