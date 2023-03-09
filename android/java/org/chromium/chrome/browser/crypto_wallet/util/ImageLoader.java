/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.MAX_BITMAP_SIZE_FOR_DOWNLOAD;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Base64;
import android.webkit.URLUtil;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.Priority;
import com.bumptech.glide.RequestBuilder;
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.load.resource.bitmap.BitmapTransformation;
import com.bumptech.glide.load.resource.bitmap.CenterInside;
import com.bumptech.glide.load.resource.bitmap.CircleCrop;
import com.bumptech.glide.load.resource.bitmap.FitCenter;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.load.resource.gif.GifDrawable;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.ImageViewTarget;
import com.bumptech.glide.request.target.Target;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.xml.sax.InputSource;

import org.chromium.chrome.browser.WebContentsFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.image_fetcher.ImageFetcher;
import org.chromium.components.image_fetcher.ImageFetcher.Params;
import org.chromium.components.image_fetcher.ImageFetcherConfig;
import org.chromium.components.image_fetcher.ImageFetcherFactory;
import org.chromium.content_public.browser.WebContents;
import org.chromium.url.GURL;

import java.io.StringReader;
import java.io.StringWriter;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

public class ImageLoader {
    private static final List<String> ANIMATED_LIST = Arrays.asList(".gif");
    private static final String UNUSED_CLIENT_NAME = "unused";
    private static final String BASE64_ENCODING_PATTERN =
            "^([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{4}|[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)$";

    /**
     * Downloads an image from a given URL, including support for GIF and SVG image types.
     * @param url URL of the image to download.
     * @param context Android context used by Glide for applying transofrmations.
     * @param isCircular When {@code true}, a circular transformation will be applied.
     * @param imageView ImageView where the downloaded image will be set.
     * @param callback Callback used to notify if the image has been set correctly. It can be {@code
     *         null}.
     */
    public static void downloadImage(String url, final Context context, final boolean isCircular,
            final ImageView imageView, final Callback callback) {
        if (!isValidImgUrl(url)) {
            if (callback != null) callback.onLoadFailed();
            return;
        }

        Profile profile = Utils.getProfile(false);
        if (isSvg(url)) {
            final String validUrl;
            if (URLUtil.isDataUrl(url)) {
                if (isBase64Encoded(url)) {
                    String decodedUrl = decodeBase64SvgUrl(url);
                    validUrl = sanitizeSvg(decodedUrl);
                } else if (isUtf8(url)) {
                    validUrl = sanitizeSvg(url);
                } else {
                    // Unsupported URL type.
                    if (callback != null) callback.onLoadFailed();
                    return;
                }
                if (TextUtils.isEmpty(validUrl)) {
                    // This may happen for invalid, or corrupted URLs.
                    if (callback != null) callback.onLoadFailed();
                    return;
                }
            } else {
                validUrl = url;
            }

            WebContentsFactory webContentsFactory = new WebContentsFactory();
            WebContents webContents =
                    webContentsFactory.createWebContentsWithWarmRenderer(profile, false);
            webContents.downloadImage(new GURL(validUrl), // Url
                    true, // isFavIcon - Does not send or accept cookies when `true`.
                    WalletConstants.MAX_BITMAP_SIZE_FOR_DOWNLOAD, // maxBitmapSize
                    false, // bypassCache
                    (id, httpStatusCode, imageUrl, bitmaps, originalImageSizes) -> { // callback
                        ImageFetcherFacade imageFetcherFacade;
                        Iterator<Bitmap> iterBitmap = bitmaps.iterator();
                        Iterator<Rect> iterSize = originalImageSizes.iterator();
                        Bitmap bestBitmap = null;
                        Rect bestSize = new Rect(0, 0, 0, 0);
                        while (iterBitmap.hasNext() && iterSize.hasNext()) {
                            Bitmap bitmap = iterBitmap.next();
                            Rect size = iterSize.next();
                            if (size.width() > bestSize.width()
                                    && size.height() > bestSize.height()) {
                                bestBitmap = bitmap;
                                bestSize = size;
                            }
                        }
                        if (bestSize.width() == 0 || bestSize.height() == 0) {
                            imageFetcherFacade = null;
                        } else {
                            BitmapDrawable bitmapDrawable = new BitmapDrawable(bestBitmap);
                            imageFetcherFacade = new ImageFetcherFacade(bitmapDrawable);
                        }
                        loadImage(imageFetcherFacade, context, isCircular, imageView, callback);
                    });
        } else {
            ImageFetcher imageFetcher = ImageFetcherFactory.createImageFetcher(
                    ImageFetcherConfig.NETWORK_ONLY, profile.getProfileKey());
            if (isGif(url)) {
                imageFetcher.fetchGif(
                        Params.create(new GURL(url), UNUSED_CLIENT_NAME), gifImage -> {
                            ImageFetcherFacade imageFetcherFacade =
                                    new ImageFetcherFacade(gifImage.getData());
                            loadImage(imageFetcherFacade, context, isCircular, imageView, callback);
                        });
            } else {
                imageFetcher.fetchImage(
                        Params.create(new GURL(url), UNUSED_CLIENT_NAME), bitmap -> {
                            BitmapDrawable bitmapDrawable = new BitmapDrawable(bitmap);
                            ImageFetcherFacade imageFetcherFacade =
                                    new ImageFetcherFacade(bitmapDrawable);
                            loadImage(imageFetcherFacade, context, isCircular, imageView, callback);
                        });
            }
        }
    }

    /**
     * Parses an input string as an XML document, manipulates it to add the missing attributes to
     * the svg tag, and then serializes it back to a string. If the input string is not a valid SVG
     * image, the method returns null.
     * <p>
     * Note that this implementation assumes that the input string is a valid SVG image and does
     * not perform any further validation or sanitation of the contents of the image.
     *
     * @param input Input string to sanitize.
     * @return sanitized string with 'width' and 'height' attributes set in the 'svg' tag.
     */
    public static String sanitizeSvg(String input) {
        if (input == null) return null;
        try {
            // Convert the input string to an XML document.
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document doc = builder.parse(new InputSource(new StringReader(input)));
            doc.getDocumentElement().normalize();

            // Check if the root element is an SVG tag.
            Element svg = doc.getDocumentElement();
            if (!svg.getNodeName().equalsIgnoreCase("svg")) {
                // Input is not an SVG image.
                return null;
            }

            // Check if the SVG tag has width and height attributes.
            String width = svg.getAttribute("width");
            String height = svg.getAttribute("height");
            if (width.isEmpty()) {
                svg.setAttribute("width", "1000px");
            }
            if (height.isEmpty()) {
                svg.setAttribute("height", "1000px");
            }

            // Serialize the XML document to a string and return it.
            TransformerFactory tf = TransformerFactory.newInstance();
            Transformer transformer = tf.newTransformer();
            StringWriter writer = new StringWriter();
            transformer.transform(new DOMSource(doc), new StreamResult(writer));
            return "data:image/svg+xml;utf8," + writer.getBuffer().toString();
        } catch (Exception e) {
            return null;
        }
    }

    private static void loadImage(ImageFetcherFacade imageFetcherFacade, Context context,
            boolean isCircular, ImageView imageView, Callback callback) {
        if (imageFetcherFacade == null
                || (imageFetcherFacade.data == null && imageFetcherFacade.drawable == null)) {
            if (callback != null) callback.onLoadFailed();
            return;
        }
        Glide.with(context)
                .load(imageFetcherFacade.data != null ? imageFetcherFacade.data
                                                      : imageFetcherFacade.drawable)
                .transform(getTransformations(isCircular))
                .diskCacheStrategy(DiskCacheStrategy.NONE)
                .priority(Priority.IMMEDIATE)
                .listener(new RequestListener<Drawable>() {
                    @Override
                    public boolean onLoadFailed(GlideException glideException, Object model,
                            Target<Drawable> target, boolean isFirstResource) {
                        return callback != null && callback.onLoadFailed();
                    }

                    @Override
                    public boolean onResourceReady(Drawable resource, Object model,
                            Target<Drawable> target, DataSource dataSource,
                            boolean isFirstResource) {
                        return callback != null && callback.onResourceReady(resource, target);
                    }
                })
                .into(imageView);
    }

    private static BitmapTransformation[] getTransformations(boolean isCircular) {
        if (isCircular) {
            return new BitmapTransformation[] {
                    new FitCenter(), new RoundedCorners(32), new CircleCrop()};
        }
        return new BitmapTransformation[] {new FitCenter(), new RoundedCorners(32)};
    }

    /**
     * Returns {@code true} if a given URL is supported for download.
     * @param url Give URL to check
     * @return {@code true} if a given URL is supported for download, {@code false} otherwise.
     */
    public static boolean isSupported(String url) {
        return isValidImgUrl(url);
    }

    /**
     * Checks if the given URL string represents an SVG image file.
     * @param url a string representing a URL to an image file.
     * @return {@code true} if the URL string represents an SVG image file, {@code false} otherwise.
     */
    public static boolean isSvg(String url) {
        // Checks if the URL is a valid image URL.
        if (!isValidImgUrl(url)) return false;
        // Converts the URL to lowercase to make the matching case-insensitive.
        url = url.toLowerCase(Locale.ENGLISH);
        // Checks if the URL starts with "data:image/svg" or ends with ".svg".
        return url.startsWith("data:image/svg") || url.endsWith(".svg");
    }

    /**
     * Checks if the given URL string represents an SVG image file.
     * @param url a string representing a URL to an image file.
     * @return {@code true} if the URL string represents an SVG image file, {@code false} otherwise.
     */
    public static boolean isGif(String url) {
        // Checks if the URL is a valid image URL.
        if (!isValidImgUrl(url)) return false;
        // Converts the URL to lowercase to make the matching case-insensitive.
        url = url.toLowerCase(Locale.ENGLISH);
        // Checks if the URL ends with ".gif".
        return url.endsWith(".gif");
    }

    private static boolean isValidImgUrl(String url) {
        // Only "data:" or HTTPS URLs.
        return URLUtil.isDataUrl(url) || URLUtil.isHttpsUrl(url);
    }

    private static String decodeBase64SvgUrl(String url) {
        String dataSection = getDataSection(url);

        if (dataSection.isEmpty() || !isBase64(dataSection)) {
            // Not a valid base64 encoded string.
            return null;
        }
        try {
            byte[] data = Base64.decode(dataSection, Base64.DEFAULT);
            ByteBuffer byteBuffer = ByteBuffer.wrap(data);
            String decoded = new String(byteBuffer.array(), StandardCharsets.UTF_8);
            return decoded;
        } catch (Exception ex) {
            return null;
        }
    }

    private static boolean isBase64(String input) {
        Pattern base64 = Pattern.compile(BASE64_ENCODING_PATTERN);
        Matcher matcher = base64.matcher(input);
        return matcher.find();
    }

    private static boolean isBase64Encoded(String url) {
        String encodingType = extractEncodingType(url);
        return encodingType != null && encodingType.startsWith("base64");
    }

    private static boolean isUtf8(String url) {
        String encodingType = extractEncodingType(url);
        return encodingType != null && encodingType.startsWith("utf8");
    }

    private static String extractEncodingType(String input) {
        int startOfEncodingSection = input.indexOf(';');
        if (startOfEncodingSection == -1 || startOfEncodingSection == input.length() - 1) {
            return null;
        }
        return input.substring(startOfEncodingSection + 1).toLowerCase(Locale.ENGLISH);
    }

    private static String getDataSection(String input) {
        // See https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/Data_URIs.
        int startOfBase64Section = input.indexOf(',');
        if (startOfBase64Section == -1 || startOfBase64Section == input.length() - 1) {
            return "";
        }
        return input.substring(startOfBase64Section + 1);
    }

    /**
     * Callback used to notify if the image has been downloaded successfully.
     * @see #downloadImage(String, Context, boolean, ImageView, Callback) /
     */
    public interface Callback {
        boolean onLoadFailed();
        boolean onResourceReady(Drawable resource, Target<Drawable> target);
    }

    private static class ImageFetcherFacade {
        final byte[] data;
        final Drawable drawable;
        public ImageFetcherFacade(byte[] data) {
            this.data = data;
            this.drawable = null;
        }

        ImageFetcherFacade(Drawable drawable) {
            this.drawable = drawable;
            this.data = null;
        }
    }
}
