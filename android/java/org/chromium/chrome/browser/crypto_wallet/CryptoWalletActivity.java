/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import android.app.SearchManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SearchView;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import androidx.viewpager.widget.ViewPager;

import org.chromium.chrome.R;

import com.google.android.material.bottomnavigation.BottomNavigationView;

public class CryptoWalletActivity extends AppCompatActivity {

    private BottomNavigationView navigation;
    private Toolbar toolbar;

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
        toolbar.setTitleTextColor(getResources().getColor(android.R.color.black, null));
        toolbar.setOverflowIcon(ContextCompat.getDrawable(this, R.drawable.ic_baseline_more_vert_24));
        setSupportActionBar(toolbar);
        ViewPager viewPager = findViewById(R.id.view_pager);
        WalletNavigationFragmentPageAdapter adapter = new WalletNavigationFragmentPageAdapter(getSupportFragmentManager());
        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
        navigation = findViewById(R.id.navigation);
        navigation.setOnNavigationItemSelectedListener(item -> {
            toolbar.setTitle(item.getTitle());
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
                viewPager.setCurrentItem(2);
                initTitle(2);
                return true;
            } else if (itemId == R.id.navigation_cards) {
                viewPager.setCurrentItem(3);
                initTitle(3);
                return true;
            } else if (itemId == R.id.navigation_lock) {
                viewPager.setCurrentItem(4);
                initTitle(4);
                return true;
            }
            return false;
        });
        initTitle(0);
    }

    private void initTitle(int position) {
        toolbar.post(() -> toolbar.setTitle(String.format(getResources().getString(R.string.my), navigation.getMenu().getItem(position).getTitle())));
    }
}
