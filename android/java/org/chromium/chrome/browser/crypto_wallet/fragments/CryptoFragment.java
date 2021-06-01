/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.content.DialogInterface;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AddAccountOnboardingDialog;
import org.chromium.chrome.browser.crypto_wallet.CryptoWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.RecoveryPhraseAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.ItemOffsetDecoration;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Executor;

public class CryptoFragment extends Fragment {
    private final List<String> titles =
            new ArrayList<>(Arrays.asList("PORTFOLIO", "PRICES", "DEFI", "NFTS", "ACCOUNTS"));

    private final List<String> recoveryPhrases =
            new ArrayList<>(Arrays.asList("Tomato", "Green", "Velvet", "Span", "Celery", "Atoms",
                    "Parent", "Stop", "Bowl", "Wishful", "Stone", "Exercise"));
    private CryptoWalletActivity.OnFinishOnboarding onFinishOnboarding;

    private View rootView;
    private View secureCryptoLayout;
    private View backupWalletLayout;

    public void setOnLastPageClick(CryptoWalletActivity.OnFinishOnboarding onFinishOnboarding) {
        this.onFinishOnboarding = onFinishOnboarding;
    }

    public static Fragment newInstance() {
        return new CryptoFragment();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        rootView = inflater.inflate(R.layout.fragment_crypto, container, false);
        return rootView;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        if (CryptoWalletActivity.isOnboardingDone) {
            setCryptoLayout();
        } else {
            setCryptoOnboardingLayout();
        }
    }

    private void setCryptoOnboardingLayout() {
        View crytpoLayout = rootView.findViewById(R.id.crypto_layout);
        View crytpoOnboardingLayout = rootView.findViewById(R.id.crypto_onboarding_layout);
        View setUpCryptoLayout = rootView.findViewById(R.id.setup_crypto_layout_id);
        View recoveryPhraseLayout = rootView.findViewById(R.id.recovery_phrase_layout_id);

        crytpoOnboardingLayout.setVisibility(View.VISIBLE);
        crytpoLayout.setVisibility(View.GONE);

        secureCryptoLayout = rootView.findViewById(R.id.secure_crypto_layout_id);
        backupWalletLayout = rootView.findViewById(R.id.backup_wallet_layout_id);
        Button backupWalletButton = rootView.findViewById(R.id.btn_backup_wallet_continue);
        backupWalletButton.setOnClickListener(v -> {
            backupWalletLayout.setVisibility(View.GONE);
            recoveryPhraseLayout.setVisibility(View.VISIBLE);
        });
        CheckBox backupWalletCheckbox = rootView.findViewById(R.id.backup_wallet_checkbox);
        backupWalletCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                backupWalletButton.setEnabled(true);
                backupWalletButton.setAlpha(1.0f);
            } else {
                backupWalletButton.setEnabled(false);
                backupWalletButton.setAlpha(0.5f);
            }
        });
        TextView backupWalletSkipButton = rootView.findViewById(R.id.btn_backup_wallet_skip);
        backupWalletSkipButton.setOnClickListener(onSkipClickListener);

        Button recoveryPhraseButton = rootView.findViewById(R.id.btn_recovery_phrase_continue);
        CheckBox recoveryPhraseCheckbox = rootView.findViewById(R.id.recovery_phrase_checkbox);
        recoveryPhraseCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                recoveryPhraseButton.setEnabled(true);
                recoveryPhraseButton.setAlpha(1.0f);
            } else {
                recoveryPhraseButton.setEnabled(false);
                recoveryPhraseButton.setAlpha(0.5f);
            }
        });
        TextView recoveryPhraseSkipButton = rootView.findViewById(R.id.btn_recovery_phrase_skip);
        recoveryPhraseSkipButton.setOnClickListener(onSkipClickListener);

        EditText passwordEdittext = rootView.findViewById(R.id.secure_crypto_password);
        EditText retypePasswordEdittext = rootView.findViewById(R.id.secure_crypto_retype_password);

        Button setupCryptoButton = rootView.findViewById(R.id.btn_setup_crypto);
        setupCryptoButton.setOnClickListener(v -> {
            setUpCryptoLayout.setVisibility(View.GONE);
            secureCryptoLayout.setVisibility(View.VISIBLE);
        });

        Button secureCryptoButton = rootView.findViewById(R.id.btn_secure_crypto_continue);
        secureCryptoButton.setOnClickListener(v -> {
            if (TextUtils.isEmpty(passwordEdittext.getText())
                    || passwordEdittext.getText().toString().length() < 7) {
                passwordEdittext.setError(getResources().getString(R.string.password_error));
            } else if (TextUtils.isEmpty(retypePasswordEdittext.getText())
                    || !passwordEdittext.getText().toString().equals(
                            retypePasswordEdittext.getText().toString())) {
                retypePasswordEdittext.setError(
                        getResources().getString(R.string.retype_password_error));
            } else {
                showFingerprintDialog(authenticationCallback);
            }
        });

        recoveryPhraseButton.setOnClickListener(v -> {
            onFinishOnboarding.onFinish();
            showAddAccountDialog();
        });

        setupRecoveryPhraseRecyclerView(rootView);
    }

    private void showAddAccountDialog() {
        AddAccountOnboardingDialog addAccountOnboardingDialog = new AddAccountOnboardingDialog();
        addAccountOnboardingDialog.setCancelable(false);
        assert getActivity() != null;
        addAccountOnboardingDialog.show(
                ((FragmentActivity) getActivity()).getSupportFragmentManager(),
                "AddAccountOnboardingDialog");
    }

    View.OnClickListener onSkipClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            onFinishOnboarding.onFinish();
        }
    };

    private void showFingerprintDialog(
            @NonNull final BiometricPrompt.AuthenticationCallback authenticationCallback) {
        assert getActivity() != null;
        Executor executor = ContextCompat.getMainExecutor(getActivity());
        new BiometricPrompt.Builder(getActivity())
                .setTitle(getResources().getString(R.string.enable_fingerprint_unlock))
                .setDescription(getResources().getString(R.string.enable_fingerprint_text))
                .setNegativeButton(getResources().getString(android.R.string.cancel), executor,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                secureCryptoLayout.setVisibility(View.GONE);
                                backupWalletLayout.setVisibility(View.VISIBLE);
                                authenticationCallback.onAuthenticationError(
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED,
                                        "User canceled the scanning process by pressing the negative button");
                            }
                        })
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }

    BiometricPrompt.AuthenticationCallback
            authenticationCallback = new BiometricPrompt.AuthenticationCallback() {
        @Override
        public void onAuthenticationSucceeded(BiometricPrompt.AuthenticationResult result) {
            super.onAuthenticationSucceeded(result);
            Toast.makeText(getActivity(), "Authentication succeeded!", Toast.LENGTH_SHORT).show();
            secureCryptoLayout.setVisibility(View.GONE);
            backupWalletLayout.setVisibility(View.VISIBLE);
        }

        @Override
        public void onAuthenticationError(int errorCode, CharSequence errString) {
            super.onAuthenticationError(errorCode, errString);
            switch (errorCode) {
                case BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED:
                    Toast.makeText(getActivity(), errString, Toast.LENGTH_SHORT).show();
                    break;
                case BiometricPrompt.BIOMETRIC_ERROR_HW_NOT_PRESENT:
                case BiometricPrompt.BIOMETRIC_ERROR_HW_UNAVAILABLE:
                    Toast.makeText(getActivity(),
                                 "Device doesn't have the supported fingerprint hardware.",
                                 Toast.LENGTH_SHORT)
                            .show();
                    break;
                case BiometricPrompt.BIOMETRIC_ERROR_NO_BIOMETRICS:
                    Toast.makeText(getActivity(), "User did not register any fingerprints.",
                                 Toast.LENGTH_SHORT)
                            .show();
                    break;
                default:
                    Toast.makeText(getActivity(), "unrecoverable error", Toast.LENGTH_SHORT).show();
            }
        }
    };

    private void setupRecoveryPhraseRecyclerView(View view) {
        RecyclerView recyclerView = view.findViewById(R.id.recovery_phrase_recyclerview);
        assert getActivity() != null;
        recyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        recyclerView.setLayoutManager(layoutManager);

        RecoveryPhraseAdapter recoveryPhraseAdapter = new RecoveryPhraseAdapter();
        recoveryPhraseAdapter.setRecoveryPhraseList(recoveryPhrases);
        recyclerView.setAdapter(recoveryPhraseAdapter);
    }

    private void setCryptoLayout() {
        View crytpoLayout = rootView.findViewById(R.id.crypto_layout);
        View crytpoOnboardingLayout = rootView.findViewById(R.id.crypto_onboarding_layout);
        crytpoOnboardingLayout.setVisibility(View.GONE);
        crytpoLayout.setVisibility(View.VISIBLE);

        ViewPager viewPager = rootView.findViewById(R.id.view_pager);
        CryptoFragmentPageAdapter adapter =
                new CryptoFragmentPageAdapter(getChildFragmentManager());
        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
        TabLayout tabLayout = rootView.findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    private class CryptoFragmentPageAdapter extends FragmentPagerAdapter {
        public CryptoFragmentPageAdapter(FragmentManager fm) {
            super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        }

        @NonNull
        @Override
        public Fragment getItem(int position) {
            return CryptoChildFragment.newInstance();
        }

        @Override
        public int getCount() {
            return 5;
        }

        @Nullable
        @Override
        public CharSequence getPageTitle(int position) {
            return titles.get(position);
        }
    }
}
