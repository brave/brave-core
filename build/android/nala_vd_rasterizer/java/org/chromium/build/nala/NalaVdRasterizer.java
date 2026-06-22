/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.build.nala;

import com.android.ide.common.vectordrawable.VdPreview;
import com.android.ide.common.vectordrawable.VdPreview.TargetSize;

import java.awt.geom.AffineTransform;
import java.awt.image.AffineTransformOp;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import javax.imageio.ImageIO;

/**
 * Rasterizes an Android VectorDrawable (.xml) to a density PNG using Android's own VdPreview
 * renderer (from com.android.tools:sdk-common) - the same code path the framework uses - so the
 * generated raster matches the vector that the Nala override mechanism applies elsewhere.
 *
 * <p>It is the mechanism for overriding upstream icons that ship as raster PNGs: rather than
 * checking in hand-produced PNGs, we generate them from the Nala vector at build time, keeping the
 * vector as the single source of truth. Real PNGs (not a vector placed in a density bucket) are
 * required because such icons may be consumed as raster bitmaps - e.g. via
 * BitmapFactory.decodeResource(), which returns null for a vector and crashes - and generating them
 * guarantees the override behaves exactly like the upstream raster for every consumer.
 *
 * <p>Usage: NalaVdRasterizer <input.xml> <output.png> <scale> <mirror> [<output.png> <scale>
 * <mirror> ...] Renders the input once per (output, scale, mirror) triple so every density bucket
 * is produced in a single JVM invocation. scale is the density factor relative to the vector's
 * intrinsic dp size (1=mdpi, 1.5=hdpi, 2=xhdpi, 3=xxhdpi, 4=xxxhdpi). mirror is "1" to flip the
 * render horizontally (for ldrtl/RTL buckets, matching how upstream ships mirrored ldrtl PNGs) or
 * "0" to leave it as-is.
 */
public final class NalaVdRasterizer {
    private NalaVdRasterizer() {}

    public static void main(String[] args) throws Exception {
        if (args.length < 4 || (args.length - 1) % 3 != 0) {
            throw new IllegalArgumentException(
                    "Usage: NalaVdRasterizer <input.xml> <output.png> <scale> "
                            + "<mirror> [<output.png> <scale> <mirror> ...]");
        }
        String xml = Files.readString(Path.of(args[0]));
        // VdParser cannot resolve "@color/..." resource references at build time
        // (there is no resource table), so bake a literal opaque-white fill in.
        // White yields a tintable template: every raster consumer of these
        // icons re-tints at runtime (e.g. ImageViewCompat.setImageTintList), so
        // only the alpha coverage matters and the baked color is irrelevant.
        xml = xml.replaceAll("@(android:)?color/[A-Za-z0-9_.]+", "#FFFFFFFF");

        for (int i = 1; i < args.length; i += 3) {
            Path output = Path.of(args[i]);
            double scale = Double.parseDouble(args[i + 1]);
            boolean mirror = "1".equals(args[i + 2]);
            StringBuilder errorLog = new StringBuilder();
            BufferedImage image =
                    VdPreview.getPreviewFromVectorXml(
                            TargetSize.createFromScale(scale), xml, errorLog);
            if (image == null) {
                throw new IOException("Failed to render " + args[0] + ": " + errorLog);
            }
            if (mirror) {
                image = flipHorizontally(image);
            }
            if (!ImageIO.write(image, "PNG", output.toFile())) {
                throw new IOException("No PNG writer available for " + output);
            }
        }
    }

    // Mirrors an image across its vertical axis. The transform is an exact pixel
    // remap, so nearest-neighbor sampling is lossless.
    private static BufferedImage flipHorizontally(BufferedImage image) {
        AffineTransform tx = AffineTransform.getScaleInstance(-1.0, 1.0);
        tx.translate(-image.getWidth(), 0);
        AffineTransformOp op = new AffineTransformOp(tx, AffineTransformOp.TYPE_NEAREST_NEIGHBOR);
        return op.filter(image, null);
    }
}
