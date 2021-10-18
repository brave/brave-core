/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.ApplicationState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.ui.base.WindowAndroid;

public class RecoveryPhraseSaveCopyBottomSheetDialogFragment
        extends BottomSheetDialogFragment implements View.OnClickListener, WindowAndroid.IntentCallback {
    public static final String TAG_FRAGMENT = RecoveryPhraseSaveCopyBottomSheetDialogFragment.class.getName();
    LinearLayout mCopyLayout;
    LinearLayout mSaveLayout;
    private String mRecoveryPhrasesText;

    public static RecoveryPhraseSaveCopyBottomSheetDialogFragment newInstance(String recoveryPhrasesText) {
        RecoveryPhraseSaveCopyBottomSheetDialogFragment recoveryPhraseSaveCopyBottomSheetDialogFragment =
                new RecoveryPhraseSaveCopyBottomSheetDialogFragment();
        Bundle args = new Bundle();
        args.putCharSequence("recoveryPhrasesText", recoveryPhrasesText);
        recoveryPhraseSaveCopyBottomSheetDialogFragment.setArguments(args);
        return recoveryPhraseSaveCopyBottomSheetDialogFragment;
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            RecoveryPhraseSaveCopyBottomSheetDialogFragment fragment =
                    (RecoveryPhraseSaveCopyBottomSheetDialogFragment) manager.findFragmentByTag(
                            RecoveryPhraseSaveCopyBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("RecoveryPhraseSaveCopyBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mRecoveryPhrasesText = getArguments().getCharSequence("recoveryPhrasesText").toString();
    }

    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        @SuppressLint("InflateParams")
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.copy_save_bottom_sheet, null);

        mCopyLayout = view.findViewById(R.id.copy_layout);
        mCopyLayout.setOnClickListener(this);
        mSaveLayout = view.findViewById(R.id.save_layout);
        mSaveLayout.setOnClickListener(this);

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
    }

    @Override
    public void onClick(View view) {
        if (view == mSaveLayout) {
            Utils.LaunchBackupFilePicker((AsyncInitializationActivity) getActivity(), this);
        } else if (view == mCopyLayout) {
            Utils.saveTextToClipboard(getActivity(), mRecoveryPhrasesText);
        }
        dismiss();
    }

    @Override
    public void onIntentCompleted(int resultCode, Intent results) {
        if (resultCode != Activity.RESULT_OK || results == null || results.getData() == null) {
            Utils.onFileNotSelected();
            return;
        }

        new AsyncTask<Void>() {
            @Override
            protected Void doInBackground() {
                Uri uri = results.getData();
                Utils.writeTextToFile(uri, mRecoveryPhrasesText);
                return null;
            }

            @Override
            protected void onCancelled() {
                Utils.onFileNotSelected();
            }

            @Override
            protected void onPostExecute(Void v) {
                if (ApplicationStatus.getStateForApplication()
                        == ApplicationState.HAS_DESTROYED_ACTIVITIES) {
                    return;
                }

                Utils.onFileSaved();
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
}
