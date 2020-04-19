/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Button;
import android.content.Intent;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.TextPaint;
import android.content.res.Configuration;

import android.support.design.widget.BottomSheetDialogFragment;
import android.support.design.widget.BottomSheetDialog;
import android.support.design.widget.BottomSheetBehavior;
import android.view.ViewTreeObserver;

import org.chromium.chrome.R;
import org.chromium.content_public.browser.LoadUrlParams;

import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.base.ContextUtils;
import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.onboarding.BraveRewardsService;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;

import static org.chromium.ui.base.ViewUtils.dpToPx;

public class RewardsBottomSheetDialogFragment extends BottomSheetDialogFragment{
    private static final String BRAVE_TERMS_PAGE = "https://basicattentiontoken.org/user-terms-of-service/";
    private static final String BRAVE_REWARDS_LEARN_MORE = "https://brave.com/faq-rewards";

    private int ntpType;
    private NewTabPageListener newTabPageListener;

    public static RewardsBottomSheetDialogFragment newInstance() {
        return new RewardsBottomSheetDialogFragment();
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
        @Nullable ViewGroup container,
        @Nullable Bundle savedInstanceState) {
    	if (getArguments()!=null) {
    		ntpType = getArguments().getInt(SponsoredImageUtil.NTP_TYPE,1);
    	}
        return inflater.inflate(R.layout.ntp_bottom_sheet, container,false);
    }

    @Override
    public void onPause() {
        newTabPageListener.updateInteractableFlag(true);
        super.onPause();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        if(isTablet || (!isTablet && newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)) {
            getDialog().getWindow().setLayout(dpToPx(getActivity(), 400), -1);
        } else {
            getDialog().getWindow().setLayout(-1, -1);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        newTabPageListener.updateInteractableFlag(false);

        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        if(isTablet || (!isTablet && ConfigurationUtils.isLandscape(getActivity()))) {
            getDialog().getWindow().setLayout(dpToPx(getActivity(), 400), -1);
        } else {
            getDialog().getWindow().setLayout(-1, -1);
        }
    }

    public void setNewTabPageListener(NewTabPageListener newTabPageListener) {
        this.newTabPageListener = newTabPageListener;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        view.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                BottomSheetDialog dialog = (BottomSheetDialog) getDialog();
                FrameLayout bottomSheet = (FrameLayout) dialog.findViewById(android.support.design.R.id.design_bottom_sheet);
                BottomSheetBehavior behavior = BottomSheetBehavior.from(bottomSheet);
                behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
            }
        });

        ImageView btnClose = view.findViewById(R.id.ntp_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        });

        Button turnOnAdsButton = view.findViewById(R.id.btn_turn_on_ads);
        if (turnOnAdsButton != null) {
            turnOnAdsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    turnOnAds();
                    dismiss();
                }
            });
        }

        Button turnOnRewardsButton = view.findViewById(R.id.btn_turn_on_rewards);
        if (turnOnRewardsButton != null) {
            turnOnRewardsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    turnOnRewards();
                    dismiss();
                }
            });
        }

        Button learnMoreButton = view.findViewById(R.id.btn_learn_more);
        if (learnMoreButton != null) {
            learnMoreButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    braveRewardsLearnMore();
                    dismiss();
                }
            });
        }

        TextView bottomSheetTitleText= view.findViewById(R.id.ntp_bottom_sheet_title);
        TextView bottomSheetText= view.findViewById(R.id.ntp_bottom_sheet_text);

        switch (ntpType) {
            case SponsoredImageUtil.BR_OFF:
                bottomSheetTitleText.setText(getResources().getString(R.string.turn_on_brave_ads_to_claim));
                turnOnRewardsButton.setVisibility(View.VISIBLE);
                learnMoreButton.setVisibility(View.VISIBLE);
                String brOffText = String.format(getResources().getString(R.string.ntp_tos_text), getResources().getString(R.string.terms_of_service), getResources().getString(R.string.hide_sponsored_images));
                int termsOfServiceIndex = brOffText.indexOf(getResources().getString(R.string.terms_of_service));
                Spanned brOffTextSpanned = BraveRewardsHelper.spannedFromHtmlString(brOffText);
                SpannableString brOffTextSS = new SpannableString(brOffTextSpanned.toString());

                ClickableSpan tosClickableSpan = new ClickableSpan() {
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

                ClickableSpan brOffHideSponsoredImagesClickableSpan = new ClickableSpan() {
                    @Override
                    public void onClick(@NonNull View textView) {
                        hideSponsoredImages();
                        dismiss();
                    }
                    @Override
                    public void updateDrawState(@NonNull TextPaint ds) {
                        super.updateDrawState(ds);
                        ds.setUnderlineText(false);
                    }
                };

                brOffTextSS.setSpan(tosClickableSpan, termsOfServiceIndex, termsOfServiceIndex + getResources().getString(R.string.terms_of_service).length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                brOffTextSS.setSpan(brOffHideSponsoredImagesClickableSpan, (brOffTextSS.length()-1) - getResources().getString(R.string.hide_sponsored_images).length() , brOffTextSS.length()-1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                ForegroundColorSpan brOffForegroundSpan = new ForegroundColorSpan(getResources().getColor(R.color.brave_theme_color));
                ForegroundColorSpan brOffForegroundSpan2 = new ForegroundColorSpan(getResources().getColor(R.color.brave_theme_color));
                brOffTextSS.setSpan(brOffForegroundSpan, termsOfServiceIndex, termsOfServiceIndex + getResources().getString(R.string.terms_of_service).length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                brOffTextSS.setSpan(brOffForegroundSpan2, (brOffTextSS.length()-1) - getResources().getString(R.string.hide_sponsored_images).length() , brOffTextSS.length()-1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                bottomSheetText.setMovementMethod(LinkMovementMethod.getInstance());
                bottomSheetText.setText(brOffTextSS);
                break;
            case SponsoredImageUtil.BR_ON_ADS_OFF:
                bottomSheetTitleText.setText(getResources().getString(R.string.turn_on_brave_ads_to_claim));
                turnOnAdsButton.setVisibility(View.VISIBLE);
                String brOnAdsOffText = String.format( getResources().getString(R.string.br_on_ads_on_text2), getResources().getString(R.string.hide_sponsored_images));
                Spanned brOnAdsOffSpanned = BraveRewardsHelper.spannedFromHtmlString(brOnAdsOffText);
                SpannableString brOnAdsOffSS = new SpannableString(brOnAdsOffSpanned.toString());

                ClickableSpan brOnAdsOffHideSponsoredImagesClickableSpan = new ClickableSpan() {
                    @Override
                    public void onClick(@NonNull View textView) {
                        hideSponsoredImages();
                        dismiss();
                    }
                    @Override
                    public void updateDrawState(@NonNull TextPaint ds) {
                        super.updateDrawState(ds);
                        ds.setUnderlineText(false);
                    }
                };

                brOnAdsOffSS.setSpan(brOnAdsOffHideSponsoredImagesClickableSpan, (brOnAdsOffSS.length()-1) - getResources().getString(R.string.hide_sponsored_images).length() , brOnAdsOffSS.length()-1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                ForegroundColorSpan brOnAdsOffForegroundSpan = new ForegroundColorSpan(getResources().getColor(R.color.brave_theme_color));
                brOnAdsOffSS.setSpan(brOnAdsOffForegroundSpan, (brOnAdsOffSS.length()-1) - getResources().getString(R.string.hide_sponsored_images).length() , brOnAdsOffSS.length()-1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                bottomSheetText.setMovementMethod(LinkMovementMethod.getInstance());
                bottomSheetText.setText(brOnAdsOffSS);
                break;
            case SponsoredImageUtil.BR_ON_ADS_ON:
                bottomSheetTitleText.setText(getResources().getString(R.string.you_are_getting_paid));
                String brOnAdsOnText = String.format(getResources().getString(R.string.br_on_ads_on_text), getResources().getString(R.string.learn_more), getResources().getString(R.string.hide_sponsored_images));
                Spanned brOnAdsOnSpanned = BraveRewardsHelper.spannedFromHtmlString(brOnAdsOnText);
                SpannableString brOnAdsOnSS = new SpannableString(brOnAdsOnSpanned.toString());

                ClickableSpan learnMoreClickableSpan = new ClickableSpan() {
                    @Override
                    public void onClick(@NonNull View textView) {
                        braveRewardsLearnMore();
                        dismiss();
                    }
                    @Override
                    public void updateDrawState(@NonNull TextPaint ds) {
                        super.updateDrawState(ds);
                        ds.setUnderlineText(false);
                    }
                };

                ClickableSpan brOnAdsOnHideSponsoredImagesClickableSpan = new ClickableSpan() {
                    @Override
                    public void onClick(@NonNull View textView) {
                        hideSponsoredImages();
                        dismiss();
                    }
                    @Override
                    public void updateDrawState(@NonNull TextPaint ds) {
                        super.updateDrawState(ds);
                        ds.setUnderlineText(false);
                    }
                };

                brOnAdsOnSS.setSpan(learnMoreClickableSpan, 0, getResources().getString(R.string.learn_more).length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                brOnAdsOnSS.setSpan(brOnAdsOnHideSponsoredImagesClickableSpan, (brOnAdsOnSS.length()-1) - getResources().getString(R.string.hide_sponsored_images).length() , brOnAdsOnSS.length()-1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                ForegroundColorSpan brOnAdsOnForegroundSpan = new ForegroundColorSpan(getResources().getColor(R.color.brave_theme_color));
                ForegroundColorSpan brOnAdsOnForegroundSpan2 = new ForegroundColorSpan(getResources().getColor(R.color.brave_theme_color));
                brOnAdsOnSS.setSpan(brOnAdsOnForegroundSpan, 0, getResources().getString(R.string.learn_more).length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                brOnAdsOnSS.setSpan(brOnAdsOnForegroundSpan2, (brOnAdsOnSS.length()-1) - getResources().getString(R.string.hide_sponsored_images).length() , brOnAdsOnSS.length()-1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                bottomSheetText.setMovementMethod(LinkMovementMethod.getInstance());
                bottomSheetText.setText(brOnAdsOnSS);
                break;
        }
    }

    private void hideSponsoredImages() {
        BackgroundImagesPreferences.setOnPreferenceValue(BackgroundImagesPreferences.PREF_SHOW_SPONSORED_IMAGES, false);
        reloadTab();
    }

    private void turnOnRewards() {
        Intent mBraveRewardsServiceIntent = new Intent(ContextUtils.getApplicationContext(), BraveRewardsService.class);
        ContextUtils.getApplicationContext().startService(mBraveRewardsServiceIntent);
    }

    private void turnOnAds() {
        BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedProfile());
    }

    private void reloadTab() {
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            Tab currentTab = chromeTabbedActivity.getActivityTab();
            SponsoredTab sponsoredTab = TabAttributes.from(currentTab).get(String.valueOf(((TabImpl)currentTab).getId()));
            sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            TabAttributes.from(currentTab).set(String.valueOf(((TabImpl)currentTab).getId()), sponsoredTab);
            newTabPageListener.updateNTPImage();
        }
    }

    private void braveRewardsLearnMore() {
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
            LoadUrlParams loadUrlParams = new LoadUrlParams(BRAVE_REWARDS_LEARN_MORE);
            chromeTabbedActivity.getActivityTab().loadUrl(loadUrlParams);
        }
    }
}
