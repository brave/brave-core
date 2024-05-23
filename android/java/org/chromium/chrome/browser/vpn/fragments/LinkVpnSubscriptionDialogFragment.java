/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveDialogFragment;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

public class LinkVpnSubscriptionDialogFragment extends BraveDialogFragment
        implements View.OnClickListener {
    private static final String TAG = "LinkVpnSubscription";

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_link_subscription_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        BraveVpnPrefUtils.setLinkSubscriptionDialogShown(true);
        Button mDoneButton = view.findViewById(R.id.btn_done);
        mDoneButton.setOnClickListener(this);
        ImageView btnClose = view.findViewById(R.id.modal_close);
        btnClose.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.btn_done) {
            TabUtils.openURLWithBraveActivity(
                    LinkSubscriptionUtils.getBraveAccountLinkUrl(
                            InAppPurchaseWrapper.SubscriptionProduct.VPN));
        }
        dismiss();
    }
}
