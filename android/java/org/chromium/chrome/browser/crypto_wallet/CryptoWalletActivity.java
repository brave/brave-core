/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import android.app.SearchManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SearchView;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.bottomnavigation.BottomNavigationView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletNavigationFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.CardsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.CryptoFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.RewardsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.SwapBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnFinishOnboarding;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

import java.util.ArrayList;
import java.util.List;

public class CryptoWalletActivity extends AppCompatActivity {
    private BottomNavigationView bottomNavigationView;
    private Toolbar toolbar;
    private WalletNavigationFragmentPageAdapter walletNavigationFragmentPageAdapter;
    private ViewPager navigationViewPager;

    public static boolean isOnboardingDone;

    private final OnFinishOnboarding onFinishOnboarding = new OnFinishOnboarding() {
        @Override
        public void onFinish() {
            isOnboardingDone = true;
            setNavigationFragments();
        }
    };

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.wallet_search, menu);
        final MenuItem searchItem = menu.findItem(R.id.search);
        SearchView searchView = (SearchView) searchItem.getActionView();
        // EditText searchEditText =
        // searchView.findViewById(androidx.appcompat.R.id.search_src_text);
        // searchEditText.setTextColor(getResources().getColor(android.R.color.black));
        // searchEditText.setHintTextColor(getResources().getColor(android.R.color.darker_gray));
        // ImageView closeButtonImage = searchView.findViewById(R.id.search_close_btn);
        // closeButtonImage.setImageResource(R.drawable.ic_baseline_close_24);
        SearchManager searchManager = (SearchManager) getSystemService(SEARCH_SERVICE);
        searchView.setSearchableInfo(searchManager.getSearchableInfo(getComponentName()));
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_crypto_wallet);
        toolbar = findViewById(R.id.toolbar);
        toolbar.setTitleTextColor(getResources().getColor(android.R.color.black, null));
        toolbar.setOverflowIcon(
                ContextCompat.getDrawable(this, R.drawable.ic_baseline_more_vert_24));
        setSupportActionBar(toolbar);
        navigationViewPager = findViewById(R.id.navigation_view_pager);
        walletNavigationFragmentPageAdapter =
                new WalletNavigationFragmentPageAdapter(getSupportFragmentManager());
        navigationViewPager.setAdapter(walletNavigationFragmentPageAdapter);
        navigationViewPager.setOffscreenPageLimit(
                walletNavigationFragmentPageAdapter.getCount() - 1);
        bottomNavigationView = findViewById(R.id.navigation);
        bottomNavigationView.setOnNavigationItemSelectedListener(item -> {
            int itemId = item.getItemId();
            if (itemId == R.id.navigation_crypto) {
                navigationViewPager.setCurrentItem(0);
                initTitle(0);
                return true;
            } else if (itemId == R.id.navigation_rewards) {
                navigationViewPager.setCurrentItem(1);
                initTitle(1);
                return true;
            } else if (itemId == R.id.navigation_cards) {
                navigationViewPager.setCurrentItem(2);
                initTitle(2);
                return true;
            } else if (itemId == R.id.navigation_swap) {
                SwapBottomSheetDialogFragment swapBottomSheetDialogFragment =
                        SwapBottomSheetDialogFragment.newInstance();
                swapBottomSheetDialogFragment.show(
                        getSupportFragmentManager(), SwapBottomSheetDialogFragment.TAG_FRAGMENT);
                return true;
            }
            return false;
        });
        setNavigationFragments();
    }

    private void setNavigationFragments() {
        List<NavigationItem> navigationItems = new ArrayList<>();
        int navigationMenu;
        CryptoFragment cryptoFragment = CryptoFragment.newInstance();
        cryptoFragment.setOnLastPageClick(onFinishOnboarding);
        navigationItems.add(new NavigationItem(
                getResources().getString(R.string.title_crypto), cryptoFragment));
        navigationItems.add(new NavigationItem(
                getResources().getString(R.string.title_rewards), RewardsFragment.newInstance()));
        navigationItems.add(new NavigationItem(
                getResources().getString(R.string.title_cards), CardsFragment.newInstance()));
        if (isOnboardingDone) {
            findViewById(R.id.swapActionButton).setVisibility(View.VISIBLE);
            navigationMenu = R.menu.navigation;
        } else {
            findViewById(R.id.swapActionButton).setVisibility(View.GONE);
            navigationMenu = R.menu.navigation_2;
        }
        if (bottomNavigationView != null) {
            bottomNavigationView.getMenu().clear();
            bottomNavigationView.inflateMenu(navigationMenu);
        }
        if (walletNavigationFragmentPageAdapter != null) {
            walletNavigationFragmentPageAdapter.setNavigationItems(navigationItems);
            walletNavigationFragmentPageAdapter.notifyDataSetChanged();
            initTitle(0);
        }
    }

    private void initTitle(int position) {
        toolbar.post(
                ()
                        -> toolbar.setTitle(String.format(getResources().getString(R.string.my),
                                walletNavigationFragmentPageAdapter.getPageTitle(position))));
    }
}
