/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import static org.chromium.chrome.browser.util.ViewUtils.dpToPx;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Toast;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.onboarding.OnViewPagerAction;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.SearchEngineEnum;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class SearchEngineOnboardingFragment extends Fragment {
    private RadioGroup radioGroup;

    private Button btnSkip, btnNext;

    private OnViewPagerAction onViewPagerAction;

    private boolean fromSettings;

    private TemplateUrl selectedSearchEngine =
            TemplateUrlServiceFactory.get().getDefaultSearchEngineTemplateUrl();

    public SearchEngineOnboardingFragment() {
        // Required empty public constructor
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
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.get();
        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();

        for (TemplateUrl templateUrl : templateUrls) {
            if (templateUrl.getIsPrepopulated()
                    && OnboardingPrefManager.searchEngineMap.get(templateUrl.getShortName())
                            != null) {
                SearchEngineEnum searchEngineEnum =
                        OnboardingPrefManager.searchEngineMap.get(templateUrl.getShortName());

                RadioButton rdBtn = new RadioButton(getActivity());
                rdBtn.setId(searchEngineEnum.getId());
                RadioGroup.LayoutParams params = new RadioGroup.LayoutParams(
                        RadioGroup.LayoutParams.MATCH_PARENT, dpToPx(getActivity(), 56));
                rdBtn.setLayoutParams(params);
                rdBtn.setTextSize(18);
                rdBtn.setButtonDrawable(null);
                rdBtn.setPadding(dpToPx(getActivity(), 30), 0, 0, 0);
                rdBtn.setTextColor(getResources().getColor(R.color.onboarding_text_color));
                rdBtn.setBackgroundDrawable(
                        getResources().getDrawable(R.drawable.radiobutton_background));
                rdBtn.setText(templateUrl.getShortName());
                rdBtn.setCompoundDrawablesWithIntrinsicBounds(
                        getResources().getDrawable(searchEngineEnum.getIcon()), null, null, null);
                rdBtn.setCompoundDrawablePadding(dpToPx(getActivity(), 16));
                if (fromSettings) {
                    rdBtn.setClickable(false);
                }
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

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        this.onViewPagerAction = onViewPagerAction;
    }

    public void setFromSettings(boolean fromSettings) {
        this.fromSettings = fromSettings;
    }

    private void initializeViews(View root) {
        radioGroup = root.findViewById(R.id.radio_group);

        btnSkip = root.findViewById(R.id.btn_skip);
        btnNext = root.findViewById(R.id.btn_next);
    }

    private void setActions() {
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
                if (!fromSettings && selectedSearchEngine != null) {
                    BraveSearchEngineUtils.setDSEPrefs(selectedSearchEngine, false);
                    BraveSearchEngineUtils.setDSEPrefs(selectedSearchEngine, true);
                }

                assert onViewPagerAction != null;
                if (onViewPagerAction != null) onViewPagerAction.onNext();
            }
        });
    }

    private void searchEngineSelected(int position, List<TemplateUrl> templateUrls) {
        selectedSearchEngine = templateUrls.get(position);
    }
}
