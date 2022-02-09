/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class DappsBottomSheetDialogFragment
        extends BottomSheetDialogFragment implements View.OnClickListener, ConnectionErrorHandler, KeyringServiceObserver {
    public static final String TAG_FRAGMENT = DappsBottomSheetDialogFragment.class.getName();

    private View mMainView;
    private Button mbtUnlock;
    private boolean mShowOnboarding;
    private KeyringService mKeyringService;

    public static DappsBottomSheetDialogFragment newInstance() {
        return new DappsBottomSheetDialogFragment();
    }

    public void showOnboarding(boolean showOnboarding) {
        mShowOnboarding = showOnboarding;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initKeyringService();
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            DappsBottomSheetDialogFragment fragment =
                    (DappsBottomSheetDialogFragment) manager.findFragmentByTag(
                            DappsBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("DappsBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.dapps_bottom_sheet, container, false);
        int displayHeight = ConfigurationUtils.getDisplayMetrics(getActivity()).get(ConfigurationUtils.HEIGHT);

        mMainView = view;
        mbtUnlock = mMainView.findViewById(R.id.unlock);
        view.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        view.getLayoutParams().height = displayHeight / 2; // set sheet height as half of the screen
        view.requestLayout();

        TextView tvDappUrl = view.findViewById(R.id.tv_dapp_url);
        tvDappUrl.setText(getCurrentHostHttpAddress());
        updateView(view);
        mbtUnlock.setOnClickListener(v -> {
            if (mShowOnboarding) {
                openWallet();
            } else {
                // TODO(serg) make a check what view has to be shown: done?
                mKeyringService.isLocked(isLocked -> {
                    if (isLocked) {
                        openWallet();
                    } else {
                        // Todo: show option to connect account
                    }
                });
            }
            dismiss();
        });
        return view;
    }

    private void openWallet() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        assert activity != null;
        activity.openBraveWallet(true);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mKeyringService != null) {
            mKeyringService.close();
        }
    }

    @Override
    public void onClick(View view) {
        // BuySendSwapActivity.ActivityType activityType = BuySendSwapActivity.ActivityType.BUY;
        // if (view == mSendLayout) {
        //     activityType = BuySendSwapActivity.ActivityType.SEND;
        // } else if (view == mSwapLayout) {
        //     activityType = BuySendSwapActivity.ActivityType.SWAP;
        // }
        // Utils.openBuySendSwapActivity(getActivity(), activityType);
        // dismiss();
    }

    private void updateView(View view) {
        if (view == null) return;

        if (mShowOnboarding) {
            mbtUnlock.setText(getResources().getString(R.string.setup_crypto));
        } else if (mKeyringService != null) {
            mKeyringService.isLocked(isLocked -> {
                if (isLocked) {
                    mbtUnlock.setText(getResources().getString(R.string.unlock));
                } else {
                    mbtUnlock.setText(getResources().getString(R.string.continue_button));
                }
            });
        }
    }

    private String getCurrentHostHttpAddress() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null) {
            return activity.getActivityTab().getUrl().getSpec();
        }
        return "";
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        initKeyringService();
    }

    private void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
        updateView(getView());
    }
}
