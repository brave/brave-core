/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.search_engines.ConfirmationDialog;
import org.chromium.brave.browser.search_engines.CustomSearchEnginesManager;
import org.chromium.brave.browser.search_engines.CustomSearchEnginesPrefManager;
import org.chromium.brave.browser.search_engines.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.BraveTemplateUrlService;
import org.chromium.ui.widget.Toast;

import java.util.List;

public class CustomSearchEngineAdapter
        extends ListAdapter<String, CustomSearchEngineAdapter.CustomSearchEngineViewHolder> {
    private Context mContext;
    private Profile mProfile;
    private CustomSearchEnginesManager mCustomSearchEnginesManager;

    protected CustomSearchEngineAdapter(Context context, Profile profile) {
        super(DIFF_CALLBACK);
        mContext = context;
        mProfile = profile;
        mCustomSearchEnginesManager = CustomSearchEnginesManager.getInstance();
    }

    private static final DiffUtil.ItemCallback<String> DIFF_CALLBACK =
            new DiffUtil.ItemCallback<String>() {
                @Override
                public boolean areItemsTheSame(@NonNull String oldItem, @NonNull String newItem) {
                    return oldItem.equals(newItem);
                }

                @Override
                public boolean areContentsTheSame(
                        @NonNull String oldItem, @NonNull String newItem) {
                    return oldItem.equals(newItem);
                }
            };

    @Override
    public void onBindViewHolder(
            @NonNull
                    CustomSearchEngineAdapter.CustomSearchEngineViewHolder
                            customSearchEngineViewHolder,
            int position) {
        final String searchEngineKeyword = getItem(position);

        customSearchEngineViewHolder.getSearchEngineText().setText(searchEngineKeyword);

        customSearchEngineViewHolder
                .getView()
                .setOnClickListener(v -> openAddCustomSearchEngineFragment(searchEngineKeyword));
        customSearchEngineViewHolder
                .getDeleteIcon()
                .setOnClickListener(v -> removeSearchEngine(searchEngineKeyword));

        loadSearchEngineLogo(
                customSearchEngineViewHolder.getSearchEngineLogo(), searchEngineKeyword);
    }

    private void openAddCustomSearchEngineFragment(String searchEngineKeyword) {
        if (mContext == null || !(mContext instanceof FragmentActivity)) {
            return;
        }

        FragmentActivity activity = (FragmentActivity) mContext;
        FragmentManager fragmentManager = activity.getSupportFragmentManager();

        Bundle args = new Bundle();
        args.putString(CustomSearchEnginesManager.KEYWORD, searchEngineKeyword);

        AddCustomSearchEnginePreferenceFragment fragment =
                new AddCustomSearchEnginePreferenceFragment();
        fragment.setArguments(args);

        fragmentManager
                .beginTransaction()
                .replace(R.id.content, fragment)
                .addToBackStack(null)
                .commit();
    }

    private void removeSearchEngine(String searchEngineKeyword) {
        showConfirmationDialog(searchEngineKeyword);
    }

    private void loadSearchEngineLogo(ImageView logoView, String searchEngineKeyword) {
        if (mProfile != null) {
            mCustomSearchEnginesManager.loadSearchEngineLogo(
                    mProfile, logoView, searchEngineKeyword);
        }
    }

    private void showConfirmationDialog(String searchEngineKeyword) {
        ConfirmationDialog.OnConfirmationDialogListener listener =
                createDialogListener(searchEngineKeyword);
        new ConfirmationDialog()
                .showConfirmDialog(
                        mContext,
                        mContext.getString(R.string.delete_custom_search_engine_title),
                        mContext.getString(
                                R.string.delete_custom_search_engine_text, searchEngineKeyword),
                        mContext.getString(R.string.delete_action_text),
                        mContext.getString(R.string.cancel_action_text),
                        listener);
    }

    private ConfirmationDialog.OnConfirmationDialogListener createDialogListener(
            String searchEngineKeyword) {
        return new ConfirmationDialog.OnConfirmationDialogListener() {
            @Override
            public void onPositiveButtonClicked() {
                Runnable templateUrlServiceReady =
                        () -> {
                            boolean isSuccess =
                                    ((BraveTemplateUrlService)
                                                    TemplateUrlServiceFactory.getForProfile(
                                                            mProfile))
                                            .remove(searchEngineKeyword);
                            if (isSuccess) {
                                mCustomSearchEnginesManager.removeCustomSearchEngine(
                                        searchEngineKeyword);
                                List<String> customSearchEngines =
                                        CustomSearchEnginesPrefManager.getInstance()
                                                .getCustomSearchEngines();
                                submitList(customSearchEngines);
                            } else {
                                Toast.makeText(
                                                mContext,
                                                mContext.getString(
                                                        R.string.failed_to_delete_search_engine),
                                                Toast.LENGTH_SHORT)
                                        .show();
                            }
                        };
                TemplateUrlServiceFactory.getForProfile(mProfile)
                        .runWhenLoaded(templateUrlServiceReady);
            }

            @Override
            public void onNegativeButtonClicked() {
                // No action needed
            }
        };
    }

    @NonNull
    @Override
    public CustomSearchEngineAdapter.CustomSearchEngineViewHolder onCreateViewHolder(
            ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.custom_search_engine_item, parent, false);
        return new CustomSearchEngineAdapter.CustomSearchEngineViewHolder(view);
    }

    public static class CustomSearchEngineViewHolder extends RecyclerView.ViewHolder {
        private final ImageView mSearchEngineLogo;
        private final TextView mSearchEngineText;
        private final ImageView mDeleteIcon;
        private final View mView;

        CustomSearchEngineViewHolder(View itemView) {
            super(itemView);
            mView = itemView;
            mSearchEngineLogo = itemView.findViewById(R.id.search_engine_logo);
            mSearchEngineText = itemView.findViewById(R.id.search_engine_text);
            mDeleteIcon = itemView.findViewById(R.id.search_engine_delete_icon);
        }

        public ImageView getSearchEngineLogo() {
            return mSearchEngineLogo;
        }

        public TextView getSearchEngineText() {
            return mSearchEngineText;
        }

        public ImageView getDeleteIcon() {
            return mDeleteIcon;
        }

        public View getView() {
            return mView;
        }
    }
}
