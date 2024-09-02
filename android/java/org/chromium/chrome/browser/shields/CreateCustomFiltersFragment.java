/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.brave_shields.mojom.FilterListAndroidHandler;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.widget.Toast;

public class CreateCustomFiltersFragment extends BravePreferenceFragment
        implements ConnectionErrorHandler {
    public static final String BRAVE_ADBLOCK_FILTER_SYNTAX_PAGE =
            "https://support.brave.com/hc/en-us/articles/6449369961741";

    private FilterListAndroidHandler mFilterListAndroidHandler;
    private EditText mEtCustomFilters;
    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_create_custom_filters, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mPageTitle.set(getString(R.string.create_custom_filters_title));
        super.onActivityCreated(savedInstanceState);

        setData();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void setData() {
        initFilterListAndroidHandler();
        mEtCustomFilters = getView().findViewById(R.id.enter_custom_filters);
        TextView tvSummary = getView().findViewById(R.id.summary);
        String summaryText =
                String.format(
                        getResources().getString(R.string.create_custom_filter_summary),
                        getResources().getString(R.string.adblock_filter_syntax));
        Spanned summaryTextSpanned = BraveRewardsHelper.spannedFromHtmlString(summaryText);
        SpannableString summaryTextSpannableString =
                new SpannableString(summaryTextSpanned.toString());

        if (getActivity() != null) {
            NoUnderlineClickableSpan summaryTextClickableSpan =
                    new NoUnderlineClickableSpan(
                            getActivity(),
                            R.color.brave_link,
                            (textView) -> {
                                CustomTabActivity.showInfoPage(
                                        getActivity(), BRAVE_ADBLOCK_FILTER_SYNTAX_PAGE);
                            });

            BraveRewardsHelper.setSpan(
                    getActivity(),
                    summaryText,
                    summaryTextSpannableString,
                    R.string.adblock_filter_syntax,
                    summaryTextClickableSpan);
            tvSummary.setMovementMethod(LinkMovementMethod.getInstance());
            tvSummary.setText(summaryTextSpannableString);
        }

        Button saveBtn = getView().findViewById(R.id.btn_save);
        saveBtn.setOnClickListener(
                view -> {
                    updateCustomFilters();
                });
        getCustomFilters();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mFilterListAndroidHandler = null;
        initFilterListAndroidHandler();
    }

    private void initFilterListAndroidHandler() {
        if (mFilterListAndroidHandler != null) {
            return;
        }

        mFilterListAndroidHandler =
                FilterListServiceFactory.getInstance().getFilterListAndroidHandler(this);
    }

    private void getCustomFilters() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.getCustomFilters(
                    customFilters -> {
                        mEtCustomFilters.setText(customFilters);
                    });
        }
    }

    private void updateCustomFilters() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.updateCustomFilters(
                    mEtCustomFilters.getText().toString(),
                    isUpdated -> {
                        int messageId =
                                isUpdated
                                        ? R.string.saved_changes_success
                                        : R.string.brave_rewards_local_general_grant_error_title;
                        if (getActivity() != null) {
                            Toast.makeText(getActivity(), messageId, Toast.LENGTH_SHORT).show();
                        }
                    });
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        MenuItem closeItem = menu.findItem(R.id.close_menu_id);
        if (closeItem != null) {
            closeItem.setVisible(false);
        }
    }

    @Override
    public void onDestroy() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.close();
        }
        super.onDestroy();
    }
}
