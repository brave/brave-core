/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Patterns;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.google.android.material.textfield.TextInputEditText;

import org.chromium.brave_shields.mojom.FilterListAndroidHandler;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.mojom.Url;

public class AddCustomFilterListsFragment
        extends BravePreferenceFragment implements ConnectionErrorHandler {
    private FilterListAndroidHandler mFilterListAndroidHandler;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_add_custom_filter_lists, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        if (getActivity() != null) {
            getActivity().setTitle(R.string.custom_filter_list_title);
        }
        super.onActivityCreated(savedInstanceState);

        setData();
    }

    private void setData() {
        initFilterListAndroidHandler();
        TextInputEditText urlEditText = getView().findViewById(R.id.enter_url_edittext);
        urlEditText.requestFocus();

        Button addBtn = getView().findViewById(R.id.btn_add);
        addBtn.setOnClickListener(view -> {
            if (urlEditText.getText().toString().length() > 0) {
                String url = urlEditText.getText().toString().trim();
                if (Patterns.WEB_URL.matcher(url).matches()) {
                    if (mFilterListAndroidHandler != null) {
                        Url filterUrl = new Url();
                        filterUrl.url = url;
                        mFilterListAndroidHandler.createSubscription(filterUrl);
                        Intent intent = new Intent();
                        getActivity().setResult(Activity.RESULT_OK, intent);
                        getActivity().finish();
                    }
                } else {
                    urlEditText.setError(getActivity().getResources().getString(
                            R.string.custom_filter_invalid_url));
                }
            }
        });
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
