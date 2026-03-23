/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines.settings;

import android.content.Context;
import android.graphics.Typeface;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.preference.PreferenceFragmentCompat;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.custom_search_engines.ConfirmationDialog;
import org.chromium.brave.browser.custom_search_engines.CustomSearchEnginesManager;
import org.chromium.brave.browser.custom_search_engines.CustomSearchEnginesPrefManager;
import org.chromium.brave.browser.custom_search_engines.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.widget.Toast;

import java.util.List;

@NullMarked
public class CustomSearchEngineAdapter
        extends ListAdapter<String, CustomSearchEngineAdapter.CustomSearchEngineViewHolder> {
    private final Context mContext;
    private final @Nullable Profile mProfile;
    private final CustomSearchEnginesManager mCustomSearchEnginesManager;
    private @Nullable LargeIconBridge mLargeIconBridge;
    private @Nullable TemplateUrlService mTemplateUrlService;

    protected CustomSearchEngineAdapter(Context context, @Nullable Profile profile) {
        super(DIFF_CALLBACK);
        mContext = context;
        mProfile = profile;
        mCustomSearchEnginesManager = CustomSearchEnginesManager.getInstance();
        if (profile != null) {
            mLargeIconBridge = new LargeIconBridge(profile);
            mTemplateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        }
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerView recyclerView) {
        super.onDetachedFromRecyclerView(recyclerView);
        if (mLargeIconBridge != null) {
            mLargeIconBridge.destroy();
        }
    }

    private static final DiffUtil.ItemCallback<String> DIFF_CALLBACK =
            new DiffUtil.ItemCallback<String>() {
                @Override
                public boolean areItemsTheSame(String oldItem, String newItem) {
                    return oldItem.equals(newItem);
                }

                @Override
                public boolean areContentsTheSame(String oldItem, String newItem) {
                    return oldItem.equals(newItem);
                }
            };

    @Override
    public void onBindViewHolder(
            CustomSearchEngineAdapter.CustomSearchEngineViewHolder customSearchEngineViewHolder,
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
        if (!(mContext instanceof FragmentActivity)) {
            return;
        }

        FragmentActivity activity = (FragmentActivity) mContext;
        Fragment hostFragment = activity.getSupportFragmentManager().findFragmentById(R.id.content);
        if (!(hostFragment instanceof PreferenceFragmentCompat)
                || !(activity
                        instanceof PreferenceFragmentCompat.OnPreferenceStartFragmentCallback)) {
            return;
        }

        AddCustomSearchEngineItemPreference editPref =
                new AddCustomSearchEngineItemPreference(mContext);
        editPref.setFragment(AddCustomSearchEnginePreferenceFragment.class.getName());
        editPref.getExtras().putString(CustomSearchEnginesManager.KEYWORD, searchEngineKeyword);

        ((PreferenceFragmentCompat.OnPreferenceStartFragmentCallback) activity)
                .onPreferenceStartFragment((PreferenceFragmentCompat) hostFragment, editPref);
    }

    private void removeSearchEngine(String searchEngineKeyword) {
        showConfirmationDialog(searchEngineKeyword);
    }

    private void loadSearchEngineLogo(ImageView logoView, String searchEngineKeyword) {
        if (mLargeIconBridge != null && mTemplateUrlService != null) {
            mCustomSearchEnginesManager.loadSearchEngineLogo(
                    mLargeIconBridge, mTemplateUrlService, logoView, searchEngineKeyword);
        }
    }

    private void showConfirmationDialog(String searchEngineKeyword) {
        ConfirmationDialog.OnConfirmationDialogListener listener =
                createDialogListener(searchEngineKeyword);
        String formattedMessage =
                mContext.getString(R.string.delete_custom_search_engine_text, searchEngineKeyword);
        SpannableStringBuilder messageSpan = new SpannableStringBuilder(formattedMessage);
        int nameStart = formattedMessage.indexOf(searchEngineKeyword);
        if (nameStart >= 0) {
            // Include the surrounding quote characters (straight or curly)
            int boldStart = nameStart > 0 ? nameStart - 1 : nameStart;
            int boldEnd = nameStart + searchEngineKeyword.length();
            if (boldEnd < formattedMessage.length()) boldEnd++;
            messageSpan.setSpan(
                    new StyleSpan(Typeface.BOLD),
                    boldStart,
                    boldEnd,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        new ConfirmationDialog()
                .showConfirmDialog(
                        mContext,
                        mContext.getString(R.string.delete_custom_search_engine_title),
                        messageSpan,
                        mContext.getString(R.string.delete_action_text),
                        mContext.getString(R.string.cancel_action_text),
                        listener);
    }

    private ConfirmationDialog.OnConfirmationDialogListener createDialogListener(
            String searchEngineKeyword) {
        return new ConfirmationDialog.OnConfirmationDialogListener() {
            @Override
            public void onPositiveButtonClicked() {
                if (mProfile == null) {
                    return;
                }
                Runnable templateUrlServiceReady =
                        () -> {
                            // TODO(https://github.com/brave/brave-browser/issues/21837): implement
                            // the actual removal of the custom search
                            // engine from template url service.
                            boolean isRemoved = true;
                            if (isRemoved) {
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
                // no-op, but required by OnConfirmationDialogListener interface
            }
        };
    }

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
