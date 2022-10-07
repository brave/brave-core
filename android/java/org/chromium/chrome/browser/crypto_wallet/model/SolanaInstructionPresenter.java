/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.SolanaAccountMeta;
import org.chromium.brave_wallet.mojom.SolanaInstruction;
import org.chromium.brave_wallet.mojom.SolanaInstructionAccountParam;
import org.chromium.brave_wallet.mojom.SolanaInstructionParam;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemText;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class SolanaInstructionPresenter {
    private boolean isDecodedDataPresent;
    public List<SolanaInstructionAccountPresenter> mAccountDatas;
    private SolanaInstruction mSolanaInstruction;
    public boolean mIsUnknown;

    public SolanaInstructionPresenter(SolanaInstruction solanaInstruction) {
        assert solanaInstruction != null : "solanaInstruction is null";
        mIsUnknown = TransactionUtils.isSolTxUnknown(solanaInstruction.programId);
        mSolanaInstruction = solanaInstruction;
        mAccountDatas = new ArrayList<>();
        if (solanaInstruction.accountMetas != null) {
            if (!mIsUnknown) {
                isDecodedDataPresent = solanaInstruction.decodedData != null;
                boolean isAccountParamsPresent =
                        isDecodedDataPresent && solanaInstruction.decodedData.accountParams != null;
                int accountParamLen = isAccountParamsPresent
                        ? solanaInstruction.decodedData.accountParams.length
                        : 0;
                boolean isSignerPresent = false;
                for (int i = 0; i < solanaInstruction.accountMetas.length; i++) {
                    SolanaAccountMeta solanaAccountMeta = solanaInstruction.accountMetas[i];
                    SolanaInstructionAccountParam solanaInstructionAccountParam = null;
                    if (isDecodedDataPresent && isAccountParamsPresent) {
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
                                    solanaInstruction.decodedData
                                            .accountParams[accountParamLen - 1];
                        }
                    }
                    // Add all types of accounts from 'accountsMetas' with specific roles from
                    // solanaInstructionAccountParam If there are more accounts (m) than roles (n)
                    // then only add the extra accounts if last role was signers,
                    // [signers] = accounts[n-1...m]
                    if (i < accountParamLen || isSignerPresent) {
                        mAccountDatas.add(new SolanaInstructionAccountPresenter(
                                solanaAccountMeta.pubkey, solanaInstructionAccountParam));
                    }
                }
            }
        }
    }

    public List<TwoLineItemText> accountDataToList() {
        List<TwoLineItemText> twoLineItemDataSources = new ArrayList<>();
        if (mIsUnknown) {
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

    public SolanaInstruction getSolanaInstruction() {
        return mSolanaInstruction;
    }
}
