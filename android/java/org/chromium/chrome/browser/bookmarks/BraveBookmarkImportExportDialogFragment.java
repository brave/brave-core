/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveDialogFragment;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.base.DeviceFormFactor;

public class BraveBookmarkImportExportDialogFragment
        extends BraveDialogFragment implements View.OnClickListener {
    private static final String IS_IMPORT = "is_import";
    private static final String IS_SUCCESS = "is_success";

    private boolean mIsImport;
    private boolean mIsSuccess;

    public static BraveBookmarkImportExportDialogFragment newInstance(
            boolean isImport, boolean isSuccess) {
        final BraveBookmarkImportExportDialogFragment fragment =
                new BraveBookmarkImportExportDialogFragment();
        final Bundle args = new Bundle();
        args.putBoolean(IS_IMPORT, isImport);
        args.putBoolean(IS_SUCCESS, isSuccess);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (getArguments() != null) {
            mIsImport = getArguments().getBoolean(IS_IMPORT);
            mIsSuccess = getArguments().getBoolean(IS_SUCCESS);
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(
                R.layout.fragment_brave_bookmark_import_export_dialog, container, false);

        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            getDialog().getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        }
        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        ImageView imageView = view.findViewById(R.id.imageView);
        TextView tvDesc = view.findViewById(R.id.tv_bookmark_desc);

        imageView.setImageResource(mIsSuccess ? R.drawable.ic_bookmark_import_export_success
                                              : R.drawable.ic_bookmark_import_export_failed);

        if (mIsImport) {
            tvDesc.setText(mIsSuccess ? R.string.import_bookmarks_success
                                      : R.string.import_bookmarks_failed);
        } else {
            tvDesc.setText(mIsSuccess ? R.string.export_bookmarks_success
                                      : R.string.export_bookmarks_failed);
        }
        Button okBtn = view.findViewById(R.id.btn_ok);
        okBtn.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        dismiss();
        if (mIsImport) {
            Activity activity = getActivity();
            if (activity != null) {
                if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity)) {
                    TabUtils.reloadIgnoringCache();
                } else {
                    activity.finish();
                    activity.overridePendingTransition(0, 0);
                    activity.startActivity(activity.getIntent());
                    activity.overridePendingTransition(0, 0);
                }
            }
        }
    }
}
