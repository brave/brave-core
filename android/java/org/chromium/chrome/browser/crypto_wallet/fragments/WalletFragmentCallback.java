package org.chromium.chrome.browser.crypto_wallet.fragments;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.app.domain.WalletModel;

import java.util.List;

/**
 * Created by Simone on 10/24/25.
 */
@NullMarked
public interface WalletFragmentCallback {
   WalletModel getWalletModel();
}
