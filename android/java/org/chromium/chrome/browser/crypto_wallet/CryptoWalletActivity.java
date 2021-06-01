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
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

import java.util.ArrayList;
import java.util.List;

public class CryptoWalletActivity extends AppCompatActivity {
    private BottomNavigationView navigation;
    private Toolbar toolbar;
    private WalletNavigationFragmentPageAdapter walletNavigationFragmentPageAdapter;
    private ViewPager viewPager;

    public static boolean isOnboardingDone;

    public interface OnFinishOnboarding {
        void onFinish();
    }

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
        SearchManager searchManager = (SearchManager) getSystemService(SEARCH_SERVICE);
        searchView.setSearchableInfo(searchManager.getSearchableInfo(getComponentName()));
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_crypto_wallet);
        toolbar = findViewById(R.id.toolbar);
        toolbar.setTitleTextColor(getResources().getColor(android.R.color.black));
        toolbar.setOverflowIcon(
                ContextCompat.getDrawable(this, R.drawable.ic_baseline_more_vert_24));
        setSupportActionBar(toolbar);
        viewPager = findViewById(R.id.view_pager);
        walletNavigationFragmentPageAdapter =
                new WalletNavigationFragmentPageAdapter(getSupportFragmentManager());
        viewPager.setAdapter(walletNavigationFragmentPageAdapter);
        viewPager.setOffscreenPageLimit(walletNavigationFragmentPageAdapter.getCount() - 1);
        navigation = findViewById(R.id.navigation);
        navigation.setOnNavigationItemSelectedListener(item -> {
            int itemId = item.getItemId();
            if (itemId == R.id.navigation_crypto) {
                viewPager.setCurrentItem(0);
                initTitle(0);
                return true;
            } else if (itemId == R.id.navigation_rewards) {
                viewPager.setCurrentItem(1);
                initTitle(1);
                return true;
            } else if (itemId == R.id.navigation_swap) {
                SwapBottomSheetDialogFragment swapBottomSheetDialogFragment =
                        SwapBottomSheetDialogFragment.newInstance();
                swapBottomSheetDialogFragment.show(
                        getSupportFragmentManager(), SwapBottomSheetDialogFragment.TAG_FRAGMENT);
                return true;
            } else if (itemId == R.id.navigation_cards) {
                int position;
                if (isOnboardingDone)
                    position = 3;
                else
                    position = 2;
                viewPager.setCurrentItem(position);
                initTitle(position);
                return true;
            } else if (itemId == R.id.navigation_lock) {
                viewPager.setCurrentItem(4);
                initTitle(4);
                return true;
            }
            return false;
        });
        setNavigationFragments();
    }

    private void setNavigationFragments() {
        List<NavigationItem> navigationItems = new ArrayList<>();
        int navigationMenu;
        CryptoFragment cryptoFragment = (CryptoFragment) CryptoFragment.newInstance();
        cryptoFragment.setOnLastPageClick(onFinishOnboarding);
        navigationItems.add(new NavigationItem(
                getResources().getString(R.string.title_crypto), cryptoFragment));
        navigationItems.add(new NavigationItem(
                getResources().getString(R.string.title_rewards), RewardsFragment.newInstance()));
        navigationItems.add(new NavigationItem(
                getResources().getString(R.string.title_cards), CardsFragment.newInstance()));
        if (isOnboardingDone) {
            findViewById(R.id.swapActionButton).setVisibility(View.VISIBLE);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.title_cards), CardsFragment.newInstance()));
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.title_lock), CardsFragment.newInstance()));
            navigationMenu = R.menu.navigation;
        } else {
            findViewById(R.id.swapActionButton).setVisibility(View.GONE);
            navigationMenu = R.menu.navigation_2;
        }
        if (navigation != null) {
            navigation.getMenu().clear();
            navigation.inflateMenu(navigationMenu);
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
