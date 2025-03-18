/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.BraveTemplateUrlService;

import java.util.List;

public class CustomSearchEnginesPreference extends Preference
        implements CustomSearchEnginesCallback {
    private RecyclerView mRecyclerView;
    private Profile mProfile;
    private CustomSearchEngineAdapter mCustomSearchEngineAdapter;

    public CustomSearchEnginesPreference(Context context) {
        super(context);
        setLayoutResource(R.layout.custom_search_engines_preference_layout);
    }

    public CustomSearchEnginesPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        Log.e("brave_search", "CustomSearchEnginesPreference");
    }

    public void initialize(Profile profile) {
        Log.e("brave_search", "initialize(Profile profile)");
        mProfile = profile;
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        Log.e("brave_search", "onBindViewHolder");
        mRecyclerView = (RecyclerView) holder.findViewById(R.id.custom_search_engine_list);

        if (mRecyclerView != null) {
            mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        }
        mCustomSearchEngineAdapter = new CustomSearchEngineAdapter();
        mCustomSearchEngineAdapter.setCustomSearchEnginesCallback(this);
        mRecyclerView.setAdapter(mCustomSearchEngineAdapter);

        updateCustomSearchEngines();
    }

    public void updateCustomSearchEngines() {
        if (mRecyclerView == null || mCustomSearchEngineAdapter == null) {
            Log.e("brave_search", "updateCustomSearchEngines 1");
            return;
        }
        List<String> customSearchEngines = CustomSearchEnginesUtil.getCustomSearchEngines();
        for (String keyword : customSearchEngines) {
            Log.e("brave_search", "updateCustomSearchEngines : keyword : " + keyword);
        }
        mCustomSearchEngineAdapter.submitList(customSearchEngines);
    }

    @Override
    public void onSearchEngineClick(String searchEngineKeyword) {}

    @Override
    public void loadSearchEngineLogo(ImageView logoView, String searchEngineKeyword) {
        if (mProfile != null) {
            CustomSearchEnginesUtil.loadSearchEngineLogo(mProfile, logoView, searchEngineKeyword);
        }
    }

    @Override
    public void removeSearchEngine(String searchEngineKeyword) {
        showConfirmDeleteSearchEngineDialog(searchEngineKeyword);
    }

    private void showConfirmDeleteSearchEngineDialog(String searchEngineKeyword) {
        final Dialog dialog = createDialog();
        setupDialogViews(dialog, searchEngineKeyword);
        setupDialogButtons(dialog, searchEngineKeyword);
        showDialog(dialog);
    }

    private Dialog createDialog() {
        final Dialog dialog = new Dialog(getContext());
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        dialog.setContentView(R.layout.custom_search_engine_alert_dialog_layout);
        return dialog;
    }

    private void setupDialogViews(Dialog dialog, String searchEngineKeyword) {
        TextView titleTextView = dialog.findViewById(R.id.dialogTitle);
        TextView messageTextView = dialog.findViewById(R.id.dialogMessage);

        titleTextView.setText(getContext().getString(R.string.delete_custom_search_engine_title));
        messageTextView.setText(
                getContext()
                        .getString(R.string.delete_custom_search_engine_text, searchEngineKeyword));
    }

    private void setupDialogButtons(Dialog dialog, String searchEngineKeyword) {
        Button positiveButton = dialog.findViewById(R.id.positiveButton);
        Button negativeButton = dialog.findViewById(R.id.negativeButton);

        positiveButton.setText(getContext().getString(R.string.delete_action_text));
        negativeButton.setText(getContext().getString(R.string.cancel_action_text));

        positiveButton.setOnClickListener(
                v -> {
                    Runnable templateUrlServiceReady =
                            () -> {
                                Log.e("brave_search", "removeSearchEngine");
                                ((BraveTemplateUrlService)
                                                TemplateUrlServiceFactory.getForProfile(mProfile))
                                        .removeSearchEngine(searchEngineKeyword);
                                CustomSearchEnginesUtil.removeCustomSearchEngine(
                                        searchEngineKeyword);
                                updateCustomSearchEngines();
                            };
                    TemplateUrlServiceFactory.getForProfile(mProfile)
                            .runWhenLoaded(templateUrlServiceReady);
                    dialog.dismiss();
                });

        negativeButton.setOnClickListener(v -> dialog.dismiss());
    }

    private void showDialog(Dialog dialog) {
        dialog.show();
        Window window = dialog.getWindow();
        window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    }
}
