/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.crypto_wallet.modal;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupWindow;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.SysUtils;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

public class BraveWalletPanel implements DialogInterface {
    private static final String TAG = "BraveWalletPanel";
    private final View mAnchorViewHost;
    private final PopupWindow mPopupWindow;
    private ViewGroup mPopupView;
    private final AppCompatActivity mActivity;
    private OnDismissListener mOnDismissListener;
    private Button mBtnSelectedNetwork;
    protected JsonRpcService mJsonRpcService;

    public BraveWalletPanel(View anchorViewHost, OnDismissListener onDismissListener,
            JsonRpcService jsonRpcService) {
        mAnchorViewHost = anchorViewHost;
        mOnDismissListener = onDismissListener;
        mActivity = BraveActivity.getChromeTabbedActivity();
        mJsonRpcService = jsonRpcService;

        mPopupWindow = new PopupWindow(mAnchorViewHost.getContext());
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setElevation(20);

        mPopupWindow.setTouchInterceptor(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    dismiss();
                    return true;
                }
                return false;
            }
        });
        mPopupWindow.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                dismiss();
            }
        });
        setUpViews();
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        mPopupWindow.setAnimationStyle(R.style.OverflowMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            mPopupWindow.setAnimationStyle(0);
        }

        mPopupWindow.showAsDropDown(mAnchorViewHost, 0, 0);
    }

    @Override
    public void cancel() {
        dismiss();
    }

    @Override
    public void dismiss() {
        mPopupWindow.dismiss();
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(this);
        }
    }

    public void resume() {
        if (isShowing()) {
            updateState();
        }
    }

    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }

    private void updateState() {
        assert mJsonRpcService != null;
        mJsonRpcService.getChainId(CoinType.ETH, chainId -> {
            mJsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                NetworkInfo[] customNetworks = Utils.getCustomNetworks(chains);
                String strNetwork = Utils.getNetworkShortText(mActivity, chainId).toString();
                mBtnSelectedNetwork.setText(strNetwork);
            });
        });
    }

    private void setUpViews() {
        LayoutInflater inflater = LayoutInflater.from(mAnchorViewHost.getContext());
        mPopupView = (ViewGroup) inflater.inflate(R.layout.brave_wallet_panel_layout, null);

        int deviceWidth = ConfigurationUtils.getDisplayMetrics(mActivity).get("width");
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);

        mPopupWindow.setWidth((int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95)));

        mBtnSelectedNetwork = mPopupView.findViewById(R.id.btn_dapps_panel_networks);
        mBtnSelectedNetwork.setOnClickListener(v -> {
            Intent intent = new Intent(mActivity, NetworkSelectorActivity.class);
            mActivity.startActivity(intent);
        });
        mPopupWindow.setContentView(mPopupView);
        updateState();
        // TODO: show connected or disconnected account page
    }
}
