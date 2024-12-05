package org.chromium.chrome.browser.youtube;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Outline;
import android.view.View;
import android.view.ViewOutlineProvider;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import org.chromium.chrome.R;
import org.chromium.ui.base.ViewUtils;

/**
 * YouTube layout extending a linear layout used to draw
 * extra buttons to switch into fullscreen or picture in picture
 * mode.
 * The layout needs to be instantiated programmatically and will
 * take care of drawing two buttons with rounded corners to each side.
 * The buttons will provide a consistent ripple effect that will not go over their boundaries
 * and a custom outline for the elevation effect.
 */
@SuppressLint("ViewConstructor")
public class YouTubeFeaturesLayout extends LinearLayout {
   public interface Callback {
      void onFullscreenClick();
      void onPictureInPictureClick();
   }

   private static final float PADDING_DP = 8f;
   private static final float ELEVATION_DP = 2f;

   @NonNull
   private final Callback mCallback;

   public YouTubeFeaturesLayout(@NonNull final Context context, @NonNull final Callback callback) {
      super(context);
      mCallback = callback;
      setId(R.id.youtube_features_layout);
      setOrientation(HORIZONTAL);
      setBackgroundResource(R.drawable.youtube_features_background);
      final int elevationPx = ViewUtils.dpToPx(context, ELEVATION_DP);
      setElevation(elevationPx);
      // Enable outline clipping to render a nice elevation effect.
      setClipToOutline(true);
      setOutlineProvider(new ViewOutlineProvider() {
         @Override
         public void getOutline(View view, Outline outline) {
            // Implement a custom shadow based on the background provided with rounded
            // corners having a radius equals to half of the view's height.
            outline.setRoundRect(0, 0, view.getWidth(), view.getHeight(), view.getHeight()/2f);
         }
      });

      final int paddingPx = ViewUtils.dpToPx(context, PADDING_DP);

      final ImageButton fullscreenButton = new ImageButton(context);
      initLayoutButton(fullscreenButton, paddingPx, R.drawable.fullscreen_on, R.drawable.youtube_button_ripple_left);

      final ImageButton pictureInPictureButton = new ImageButton(context);
      initLayoutButton(pictureInPictureButton, paddingPx, R.drawable.picture_in_picture, R.drawable.youtube_button_ripple_right);

      fullscreenButton.setOnClickListener(v -> mCallback.onFullscreenClick());
      pictureInPictureButton.setOnClickListener(v -> mCallback.onPictureInPictureClick());

      addView(fullscreenButton);
      addView(pictureInPictureButton);
   }

   private void initLayoutButton(@NonNull final ImageButton imageButton,
                                 final int paddingPx,
                                 @DrawableRes int drawableRes,
                                 @DrawableRes final int rippleEffectDrawableRes) {
      imageButton.setImageResource(drawableRes);
      imageButton.setBackgroundResource(0);
      imageButton.setPadding(paddingPx, paddingPx, paddingPx, paddingPx);
      // Set a custom foreground to provide a ripple effect that is consistent with the
      // button shape and does not go over its boundaries.
      imageButton.setForeground(ContextCompat.getDrawable(imageButton.getContext(), rippleEffectDrawableRes));

      // Set layout weight to 0.5 to give each button half of the layout width.
      final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 0.5f);
      params.setMargins(0, 0, 0, 0);
      imageButton.setLayoutParams(params);
   }
}
