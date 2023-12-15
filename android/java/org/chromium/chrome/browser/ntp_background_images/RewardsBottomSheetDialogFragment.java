/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.content.res.Configuration;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.base.DeviceFormFactor;

public class RewardsBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private static final String TAG = "RewardsBottomSheet";

    private static final String BRAVE_TERMS_PAGE =
            "https://basicattentiontoken.org/user-terms-of-service/";
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
        if (getArguments() != null) {
            ntpType = getArguments().getInt(SponsoredImageUtil.NTP_TYPE, 1);
        }
        return inflater.inflate(R.layout.ntp_bottom_sheet, container, false);
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
        if (isTablet || (!isTablet && newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)) {
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
        if (isTablet || (!isTablet && ConfigurationUtils.isLandscape(getActivity()))) {
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

        ImageView btnClose = view.findViewById(R.id.ntp_bottom_sheet_close);
        Button turnOnAdsButton = view.findViewById(R.id.btn_turn_on_ads);
        TextView bottomSheetTitleText = view.findViewById(R.id.ntp_bottom_sheet_title);
        TextView bottomSheetText = view.findViewById(R.id.ntp_bottom_sheet_text);
        TextView bottomSheetTosText = view.findViewById(R.id.ntp_bottom_sheet_tos_text);
        TextView learnMoreText = view.findViewById(R.id.learn_more_text);
        TextView hideSponsoredImagesText = view.findViewById(R.id.hide_sponsored_images_text);

        if (btnClose != null) {
            btnClose.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    dismiss();
                }
            });
        }

        if (turnOnAdsButton != null) {
            turnOnAdsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    try {
                        BraveActivity.getBraveActivity().openRewardsPanel();
                    } catch (BraveActivity.BraveActivityNotFoundException e) {
                        Log.e(TAG, "onViewCreated turnOnAdsButton click " + e);
                    }
                    dismiss();
                }
            });
        }

        if (learnMoreText != null) {
            learnMoreText.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    TabUtils.openUrlInSameTab(BRAVE_REWARDS_LEARN_MORE);
                    dismiss();
                }
            });
        }

        if (hideSponsoredImagesText != null) {
            hideSponsoredImagesText.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    hideSponsoredImages();
                    dismiss();
                }
            });
        }

        switch (ntpType) {
        case SponsoredImageUtil.BR_ON_ADS_OFF:
            bottomSheetTitleText.setText(getResources().getString(R.string.earn_tokens_for_viewing_title));
            turnOnAdsButton.setVisibility(View.VISIBLE);

            bottomSheetTosText.setVisibility(View.VISIBLE);
            String tosText = String.format(getResources().getString(R.string.ntp_tos_text), getResources().getString(R.string.terms_of_service));
            int termsOfServiceIndex = tosText.indexOf(getResources().getString(R.string.terms_of_service));
            Spanned tosTextSpanned = BraveRewardsHelper.spannedFromHtmlString(tosText);
            SpannableString tosTextSS = new SpannableString(tosTextSpanned.toString());

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

            tosTextSS.setSpan(tosClickableSpan, termsOfServiceIndex, termsOfServiceIndex + getResources().getString(R.string.terms_of_service).length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            ForegroundColorSpan brOffForegroundSpan = new ForegroundColorSpan(getContext().getColor(R.color.brave_theme_color));
            tosTextSS.setSpan(brOffForegroundSpan, termsOfServiceIndex, termsOfServiceIndex + getResources().getString(R.string.terms_of_service).length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            bottomSheetTosText.setMovementMethod(LinkMovementMethod.getInstance());
            bottomSheetTosText.setText(tosTextSS);
            break;
        case SponsoredImageUtil.BR_ON_ADS_ON:
            bottomSheetTitleText.setText(getResources().getString(R.string.you_are_earning_tokens2));
            break;
        }
    }

    private void hideSponsoredImages() {
        BackgroundImagesPreferences.setOnPreferenceValue(BackgroundImagesPreferences.PREF_SHOW_SPONSORED_IMAGES, false);
        reloadTab();
    }

    private void reloadTab() {
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if (chromeTabbedActivity != null && chromeTabbedActivity.getActivityTab() != null) {
            Tab currentTab = chromeTabbedActivity.getActivityTab();
            SponsoredTab sponsoredTab =
                    TabAttributes.from(currentTab).get(String.valueOf(currentTab.getId()));
            sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            TabAttributes.from(currentTab).set(String.valueOf(currentTab.getId()), sponsoredTab);
            newTabPageListener.updateNTPImage();
        }
    }
}
