package org.chromium.brave.browser.customize_menu.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import org.chromium.brave.browser.customize_menu.R;

import org.chromium.build.annotations.Nullable;

/**
 * Brave custom menu preference hosted by {@link CustomizeMenuPreferenceFragment}. It shows
 * all the items that can be hidden from the main menu.
 */
public class CustomizeMenuPreference extends Preference {
   private RecyclerView mRecyclerView;

   public CustomizeMenuPreference(Context context, @Nullable AttributeSet attrs) {
      super(context, attrs);
   }

   @Override
   public void onBindViewHolder(PreferenceViewHolder holder) {
      super.onBindViewHolder(holder);

      mRecyclerView = (RecyclerView) holder.findViewById(R.id.menu_item_list);

      if (mRecyclerView != null) {
         mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
      }
   }
}
