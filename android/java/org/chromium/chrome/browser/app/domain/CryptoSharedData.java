package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;

import java.util.List;

public interface CryptoSharedData {
    int getCoinType();
    String getChainId();
    Context getContext();
    LiveData<Integer> getCoinTypeLd();
    String[] getEnabledKeyrings();
    List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes();
    LiveData<List<AccountInfo>> getAccounts();
}
