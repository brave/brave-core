package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.widget.EditText;

import androidx.annotation.StringRes;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

public class AdvanceTxSettingActivity extends BraveWalletBaseActivity {
    private static final String TAG = "AdvanceTxSettingActivity";
    private EditText mEtCustomNonce;
    private String newNonce;

    /**
     * Creates an Intent object to open the advanced transaction settings activity.
     *
     * @param context Context used to create the Intent.
     * @param txId Id of the transaction to edit.
     * @param chainId Chain Id of the transaction to edit.
     * @param nonce Current transaction nonce, in hexadecimal format.
     * @return Intent object to open the advanced transaction settings activity.
     */
    public static Intent createIntent(
            final Context context, final String txId, final String chainId, final String nonce) {
        final Intent intent = new Intent(context, AdvanceTxSettingActivity.class);
        intent.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID, txId);
        intent.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID, chainId);
        intent.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE, nonce);

        return intent;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_advance_tx_setting);
        setupBraveToolBar(R.string.brave_wallet_advanced_settings);

        mEtCustomNonce = findViewById(R.id.activity_advance_setting_et_nonce);
        String txId = getIntent().getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID);
        String chainId =
                getIntent().getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID);
        String nonce =
                getIntent().getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE);
        mEtCustomNonce.setText(Utils.hexToIntString(nonce));

        findViewById(R.id.activity_advance_setting_btn_save)
                .setOnClickListener(
                        v -> {
                            newNonce = mEtCustomNonce.getText().toString();
                            if (!TextUtils.isEmpty(newNonce)) {
                                newNonce = Utils.toHex(mEtCustomNonce.getText().toString());
                            }
                            getEthTxManagerProxy()
                                    .setNonceForUnapprovedTransaction(
                                            chainId,
                                            txId,
                                            newNonce,
                                            isSet -> {
                                                if (isSet) {
                                                    Intent result =
                                                            new Intent()
                                                                    .putExtra(
                                                                            WalletConstants
                                                                                    .ADVANCE_TX_SETTING_INTENT_RESULT_NONCE,
                                                                            newNonce);
                                                    setResult(Activity.RESULT_OK, result);
                                                    finish();
                                                } else {
                                                    Log.e(TAG, "Unable to set nonce ");
                                                }
                                            });
                        });
        onInitialLayoutInflationComplete();
    }

    private void setupBraveToolBar(@StringRes int title) {
        MaterialToolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle(title);
        setSupportActionBar(toolbar);
    }
}
