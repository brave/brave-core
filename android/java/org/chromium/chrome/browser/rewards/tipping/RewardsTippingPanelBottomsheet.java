/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;

public class RewardsTippingPanelBottomsheet extends BottomSheetDialogFragment {
    public static final String TAG_FRAGMENT = "tipping_bottomsheet_tag";

    private int mCurrentTabId = -1;
    private static final String TAG = "TippingBottomsheet";

    public static RewardsTippingPanelBottomsheet newInstance(int tabId) {
        RewardsTippingPanelBottomsheet fragment = new RewardsTippingPanelBottomsheet();
        Bundle args = new Bundle();
        args.putInt(RewardsTippingBannerActivity.TAB_ID_EXTRA, tabId);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.AppBottomSheetDialogTheme);
    }

    @Override
    public void show(@NonNull FragmentManager manager, @Nullable String tag) {
        try {
            RewardsTippingPanelBottomsheet fragment =
                    (RewardsTippingPanelBottomsheet) manager.findFragmentByTag(
                            RewardsTippingPanelBottomsheet.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    public void replace() {
        if (getArguments() != null) {
            mCurrentTabId = getArguments().getInt(RewardsTippingBannerActivity.TAB_ID_EXTRA);
        }
        RewardsTippingPanelFragment tippingPanelFragment =
                RewardsTippingPanelFragment.newInstance(mCurrentTabId);
        getChildFragmentManager()
                .beginTransaction()
                .replace(R.id.tippingPanelFragment, tippingPanelFragment)
                .commit();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstance) {
        BottomSheetDialog dialog = (BottomSheetDialog) super.onCreateDialog(savedInstance);
        final View view = LayoutInflater.from(getContext())
                                  .inflate(R.layout.brave_rewards_tippingpanel_bottomsheet, null);
        dialog.setContentView(view);
        replace();
        setupFullHeight(dialog);

        return dialog;
    }

    private void setupFullHeight(BottomSheetDialog bottomSheetDialog) {
        FrameLayout bottomSheet =
                (FrameLayout) bottomSheetDialog.findViewById(R.id.design_bottom_sheet);
        BottomSheetBehavior behavior = BottomSheetBehavior.from(bottomSheet);
        behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    }

    public static void showTippingPanelBottomSheet(AppCompatActivity activity, int tabId) {
        if (activity != null) {
            RewardsTippingPanelBottomsheet dialog =
                    RewardsTippingPanelBottomsheet.newInstance(tabId);
            dialog.show(activity.getSupportFragmentManager(), TAG_FRAGMENT);
        }
    }
}
