/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.presenters;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.SolanaAccountMeta;
import org.chromium.brave_wallet.mojom.SolanaInstruction;
import org.chromium.brave_wallet.mojom.SolanaInstructionAccountParam;
import org.chromium.brave_wallet.mojom.SolanaInstructionParam;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemHeader;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemText;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class SolanaInstructionPresenter {
    public List<SolanaInstructionAccountPresenter> mAccountDatas;
    public boolean mIsUnknown;
    private boolean isDecodedDataPresent;
    private SolanaInstruction mSolanaInstruction;
    private Integer mInstructionType;
    private String mFromPubKey;
    private String mToPubKey;

    public SolanaInstructionPresenter(SolanaInstruction solanaInstruction) {
        assert solanaInstruction != null : "solanaInstruction is null";
        mIsUnknown = TransactionUtils.isSolTxUnknown(solanaInstruction.programId);
        mSolanaInstruction = solanaInstruction;
        mAccountDatas = new ArrayList<>();
        if (solanaInstruction.accountMetas != null && !mIsUnknown) {
            isDecodedDataPresent = solanaInstruction.decodedData != null;
            boolean isAccountParamsPresent =
                    isDecodedDataPresent && solanaInstruction.decodedData.accountParams != null;
            int accountParamLen =
                    isAccountParamsPresent ? solanaInstruction.decodedData.accountParams.length : 0;
            boolean isSignerPresent = false;
            if (isDecodedDataPresent && isAccountParamsPresent) {
                for (int i = 0; i < solanaInstruction.accountMetas.length; i++) {
                    SolanaAccountMeta solanaAccountMeta = solanaInstruction.accountMetas[i];
                    SolanaInstructionAccountParam solanaInstructionAccountParam = null;
                    if (i < accountParamLen) {
                        // Add all accounts with corresponding labels (e.g. From, To) if a label
                        // exists
                        solanaInstructionAccountParam =
                                solanaInstruction.decodedData.accountParams[i];
                        isSignerPresent =
                                solanaInstructionAccountParam.name.equalsIgnoreCase("signers");
                    } else if (isSignerPresent) {
                        // Out of labels, add as signer if present
                        solanaInstructionAccountParam =
                                solanaInstruction.decodedData.accountParams[accountParamLen - 1];
                    }
                    // Add all types of accounts from 'accountsMetas' with specific roles from
                    // solanaInstructionAccountParam If there are more accounts (m) than roles (n)
                    // then only add the extra accounts if last role was signers, [signers] =
                    // accounts[n-1...m]
                    if (i < accountParamLen || isSignerPresent) {
                        mAccountDatas.add(new SolanaInstructionAccountPresenter(
                                solanaAccountMeta.pubkey, solanaInstructionAccountParam));
                    }
                }
            }
        }
    }

    public List<TwoLineItem> toTwoLineList(Context context) {
        List<TwoLineItem> twoLineItems = new ArrayList<>();
        twoLineItems.add(new TwoLineItemText(
                TransactionUtils.getSolanaProgramIdName(mSolanaInstruction.programId, context)
                        + " - "
                        + context.getString(
                                TransactionUtils.getSolTxSubType(mSolanaInstruction.programId,
                                        mSolanaInstruction.decodedData != null
                                                ? mSolanaInstruction.decodedData.instructionType
                                                : -1)),
                null));
        SolanaInstructionPresenter solanaInstructionPresenter =
                new SolanaInstructionPresenter(mSolanaInstruction);
        if (shouldShowRawData()) {
            twoLineItems.add(new TwoLineItemHeader(context.getString(R.string.accounts)));
        }
        twoLineItems.addAll(solanaInstructionPresenter.accountDataToList());

        if (shouldShowRawData()) {
            // Add data and program id field also
            twoLineItems.add(
                    new TwoLineItemHeader(context.getString(R.string.brave_wallet_data_text)));
            twoLineItems.addAll(solanaInstructionPresenter.dataToList());

            twoLineItems.add(
                    new TwoLineItemHeader(context.getString(R.string.brave_wallet_tx_progam_id)));
            twoLineItems.addAll(solanaInstructionPresenter.programIdToList());
        }
        twoLineItems.addAll(solanaInstructionPresenter.accountParamDataToList());
        return twoLineItems;
    }
    public List<TwoLineItemText> accountDataToList() {
        List<TwoLineItemText> twoLineItemDataSources = new ArrayList<>();
        if (mIsUnknown || mSolanaInstruction.decodedData == null) {
            for (SolanaAccountMeta solanaAccountMeta : getSolanaInstruction().accountMetas) {
                twoLineItemDataSources.add(new TwoLineItemText(null, solanaAccountMeta.pubkey));
            }
        } else {
            for (SolanaInstructionAccountPresenter accountPresenter : mAccountDatas) {
                twoLineItemDataSources.add(new TwoLineItemText(
                        accountPresenter.mLocalizeAccountHeader, accountPresenter.mPubKey));
            }
        }
        return twoLineItemDataSources;
    }

    public List<TwoLineItemText> accountParamDataToList() {
        List<TwoLineItemText> twoLineItemDataSources = new ArrayList<>();
        if (isDecodedDataPresent && mSolanaInstruction.decodedData.params != null) {
            for (SolanaInstructionParam instructionParam : mSolanaInstruction.decodedData.params) {
                String value = instructionParam.value;
                if (instructionParam.name.equalsIgnoreCase(WalletConstants.SOL_LAMPORTS)) {
                    value = String.format(Locale.getDefault(), "%.9f",
                                    Utils.getBalanceForCoinType(
                                            CoinType.SOL, Utils.SOL_DEFAULT_DECIMALS, value))
                            + " " + WalletConstants.SOL;
                }
                twoLineItemDataSources.add(
                        new TwoLineItemText(instructionParam.localizedName, value));
            }
        }
        return twoLineItemDataSources;
    }

    public List<TwoLineItemText> programIdToList() {
        return Arrays.asList(new TwoLineItemText(null, mSolanaInstruction.programId));
    }

    public List<TwoLineItemText> dataToList() {
        return Arrays.asList(new TwoLineItemText(null, Arrays.toString(mSolanaInstruction.data)));
    }

    // Get lamport from decoded data params
    public String getLamportAmount() {
        if (isDecodedDataPresent && mSolanaInstruction.decodedData.params != null) {
            for (SolanaInstructionParam instructionParam : mSolanaInstruction.decodedData.params) {
                if (instructionParam.name.equalsIgnoreCase(WalletConstants.SOL_LAMPORTS)) {
                    return instructionParam.value;
                }
            }
        }
        return "0";
    }

    public Integer getInstructionType() {
        if (mInstructionType != null) return mInstructionType;
        if (isDecodedDataPresent) {
            mInstructionType = mSolanaInstruction.decodedData.instructionType;
            return mInstructionType;
        }
        return null;
    }

    public String fromPubKey() {
        if (mFromPubKey != null) return mFromPubKey;
        mFromPubKey = getPubKeyPerParamKey(WalletConstants.SOL_DAPP_FROM_ACCOUNT);
        return mFromPubKey;
    }

    public String toPubKey() {
        if (mToPubKey != null) return mToPubKey;
        mToPubKey = getPubKeyPerParamKey(WalletConstants.SOL_DAPP_TO_ACCOUNT);
        return mToPubKey;
    }

    // Returns the first found account pub key from accounts meta, corresponding to input "key" from
    // accountParams
    public String getPubKeyPerParamKey(String key) {
        if (TextUtils.isEmpty(key)) return null;
        String pubKey = null;
        if (isAccountMetaPresent()) {
            SolanaInstructionAccountParam[] accountParams =
                    mSolanaInstruction.decodedData.accountParams;
            for (int i = 0; i < accountParams.length; i++) {
                if (accountParams[i].name.equalsIgnoreCase(key)) {
                    if (mSolanaInstruction.accountMetas.length > i) {
                        pubKey = mSolanaInstruction.accountMetas[i].pubkey;
                    }
                    return pubKey;
                }
            }
        }
        return pubKey;
    }

    public SolanaInstruction getSolanaInstruction() {
        return mSolanaInstruction;
    }

    public boolean isTokenInstruction() {
        return mSolanaInstruction.programId.equals(WalletConstants.SOL_INS_TOKEN);
    }

    public boolean shouldShowRawData() {
        return mIsUnknown || mSolanaInstruction.decodedData == null;
    }

    private boolean isAccountMetaPresent() {
        return isDecodedDataPresent && mSolanaInstruction.decodedData.accountParams != null
                && mSolanaInstruction.accountMetas != null;
    }

    public static class SolanaInstructionAccountPresenter {
        public String mPubKey;
        public String mLocalizeAccountHeader;
        public String mAccountHeader;

        public SolanaInstructionAccountPresenter(
                String pubKey, SolanaInstructionAccountParam solInsAccountParam) {
            this.mPubKey = pubKey;
            if (solInsAccountParam == null) {
                mLocalizeAccountHeader = "";
                mAccountHeader = "";
                return;
            }
            mLocalizeAccountHeader = solInsAccountParam.localizedName;
            mAccountHeader = solInsAccountParam.name;
        }
    }
}
