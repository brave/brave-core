/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.Spanned;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import java.util.Locale;

import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveUphold;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.R;

public class BraveRewardsVerifyWalletActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.verify_wallet_activity);
        SetVerifyWalletBtnClickHandler();
        SetUpholdLinkHandler();
    }

    void SetVerifyWalletBtnClickHandler() {
        Button btnVerifyWallet = (Button)findViewById(R.id.verify_wallet_btn);
        btnVerifyWallet.setOnClickListener( (View v) -> {
            String verify_url = getIntent().getStringExtra(BraveRewardsExternalWallet.VERIFY_URL);
            Intent intent = new Intent();
            intent.putExtra(BraveActivity.OPEN_URL, verify_url);
            setResult(RESULT_OK, intent);
            finish();
        });
    }

    void SetUpholdLinkHandler() {
        TextView uphold_link = (TextView)findViewById(R.id.service_provider_txt);
        final String part1 = getResources().getString(R.string.verify_wallet_service_note);
        final String part2 = getResources().getString(R.string.verify_wallet_uphold);
        final String built_service_str = String.format(Locale.US, "%s <b>%s</b>", part1, part2);
        Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(built_service_str);
        uphold_link.setText(toInsert);

        uphold_link.setOnTouchListener(
                (View view, MotionEvent motionEvent) -> {
                    boolean event_consumed = false;
                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                        int offset = uphold_link.getOffsetForPosition(
                                motionEvent.getX(), motionEvent.getY());

                        if (BraveRewardsHelper.subtextAtOffset(built_service_str, part2, offset) ){
                            Intent intent = new Intent();
                            intent.putExtra(BraveActivity.OPEN_URL, BraveUphold.UPHOLD_ORIGIN_URL);
                            setResult(RESULT_OK, intent);
                            finish();
                            event_consumed = true;
                        }
                    }
                    return event_consumed;
        });
    }
}
