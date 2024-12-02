/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import static android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.os.Bundle;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;

import androidx.core.content.res.ResourcesCompat;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

public class SearchEngineOnboardingFragment extends Fragment {
    private String searchSpanText = "%s\n%s";
    private RadioGroup radioGroup;

    private Button btnSave;

    private Profile mProfile;
    private TemplateUrl mSelectedSearchEngine;

    public SearchEngineOnboardingFragment() {
        // Required empty public constructor
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mProfile = ProfileManager.getLastUsedRegularProfile();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View root = inflater.inflate(R.layout.fragment_search_engine_onboarding, container, false);

        initializeViews(root);

        setActions();

        refreshData();

        return root;
    }

    private void refreshData() {
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(mProfile);
        assert templateUrlService != null; // Non-private profile shall always have the service
        if (templateUrlService == null) return;
        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                BraveSearchEngineUtils.getTemplateUrlByShortName(
                        mProfile, BraveSearchEngineUtils.getDSEShortName(mProfile, false));

        Iterator<TemplateUrl> iterator = templateUrls.iterator();
        Set<String> templateUrlSet = new HashSet<String>();
        while (iterator.hasNext()) {
            TemplateUrl templateUrl = iterator.next();
            if (!templateUrlSet.contains(templateUrl.getShortName())) {
                templateUrlSet.add(templateUrl.getShortName());
            } else {
                iterator.remove();
            }
        }

        for (TemplateUrl templateUrl : templateUrls) {
            if (templateUrl.getIsPrepopulated()
                    && OnboardingPrefManager.searchEngineMap.get(templateUrl.getShortName())
                    != null) {
                SearchEngineEnum searchEngineEnum =
                    OnboardingPrefManager.searchEngineMap.get(templateUrl.getShortName());

                String title = templateUrl.getShortName();
                String desc = getActivity().getResources().getString(searchEngineEnum.getDesc());

                SpannableString searchTextSpan =
                        new SpannableString(String.format(searchSpanText, title, desc));
                searchTextSpan.setSpan(new AbsoluteSizeSpan(16, true), 0, title.length(),
                        SPAN_EXCLUSIVE_EXCLUSIVE);
                searchTextSpan.setSpan(
                        new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), 0,
                        title.length(), SPAN_EXCLUSIVE_EXCLUSIVE);
                searchTextSpan.setSpan(new AbsoluteSizeSpan(12, true), title.length() + 1,
                        searchTextSpan.length(), SPAN_EXCLUSIVE_EXCLUSIVE);

                RadioButton rdBtn = new RadioButton(getActivity());
                rdBtn.setId(searchEngineEnum.getId());
                RadioGroup.LayoutParams params = new RadioGroup.LayoutParams(
                        RadioGroup.LayoutParams.MATCH_PARENT, dpToPx(getActivity(), 64));
                params.setMargins(0, dpToPx(getActivity(), 6), 0, 0);
                rdBtn.setLayoutParams(params);
                rdBtn.setButtonDrawable(null);
                rdBtn.setPadding(dpToPx(getActivity(), 30), 0, 0, 0);
                rdBtn.setTextColor(getActivity().getColor(R.color.onboarding_text_color));
                rdBtn.setBackgroundDrawable(
                        ResourcesCompat.getDrawable(getActivity().getResources(),
                                R.drawable.radiobutton_background, /* theme= */ null));
                rdBtn.setText(searchTextSpan);
                rdBtn.setCompoundDrawablesWithIntrinsicBounds(
                        ResourcesCompat.getDrawable(getActivity().getResources(),
                                searchEngineEnum.getIcon(), /* theme= */ null),
                        null, null, null);
                rdBtn.setCompoundDrawablePadding(dpToPx(getActivity(), 16));
                radioGroup.addView(rdBtn);
            }
        }

        if (defaultSearchEngineTemplateUrl != null
                && OnboardingPrefManager.searchEngineMap.get(
                    defaultSearchEngineTemplateUrl.getShortName())
                != null)
            radioGroup.check(OnboardingPrefManager.searchEngineMap
                             .get(defaultSearchEngineTemplateUrl.getShortName())
                             .getId());
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int i) {
                View radioButton = radioGroup.findViewById(i);
                int index = radioGroup.indexOfChild(radioButton);
                searchEngineSelected(index, templateUrls);
            }
        });
    }

    private void initializeViews(View root) {
        radioGroup = root.findViewById(R.id.radio_group);

        btnSave = root.findViewById(R.id.btn_save);
    }

    private void setActions() {
        btnSave.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (mSelectedSearchEngine == null) {
                            mSelectedSearchEngine =
                                    BraveSearchEngineUtils.getTemplateUrlByShortName(
                                            mProfile,
                                            BraveSearchEngineUtils.getDSEShortName(
                                                    mProfile, false));
                        }
                        if (mSelectedSearchEngine != null) {
                            BraveSearchEngineUtils.setDSEPrefs(mSelectedSearchEngine, mProfile);
                            BraveSearchEngineUtils.setDSEPrefs(
                                    mSelectedSearchEngine,
                                    mProfile.getPrimaryOtrProfile(/* createIfNeeded= */ true));
                        }
                        getActivity().finish();
                    }
                });
    }

    private void searchEngineSelected(int position, List<TemplateUrl> templateUrls) {
        mSelectedSearchEngine = templateUrls.get(position);
    }
}
