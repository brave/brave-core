/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/**
 * Java application that takes in an input jar, performs a series of bytecode
 * transformations, and generates an output jar.
 */
class ByteCodeProcessor {
    private static final String CLASS_FILE_SUFFIX = ".class";
    private static final String TEMPORARY_FILE_SUFFIX = ".temp";
    private static final int BUFFER_SIZE = 16384;
    private static boolean sVerbose;
    private static boolean sIsPrebuilt;
    private static boolean sShouldUseThreadAnnotations;
    private static boolean sShouldCheckClassPath;
    private static ClassLoader sDirectClassPathClassLoader;
    private static ClassLoader sFullClassPathClassLoader;
    private static Set<String> sFullClassPathJarPaths;
    private static Set<String> sMissingClassesAllowlist;
    private static ClassPathValidator sValidator;

    private static class EntryDataPair {
        private final ZipEntry mEntry;
        private final byte[] mData;

        private EntryDataPair(ZipEntry mEntry, byte[] mData) {
            this.mEntry = mEntry;
            this.mData = mData;
        }

        private static EntryDataPair create(String zipPath, byte[] data) {
            ZipEntry entry = new ZipEntry(zipPath);
            entry.setMethod(ZipEntry.STORED);
            entry.setTime(0);
            entry.setSize(data.length);
            CRC32 crc = new CRC32();
            crc.update(data);
            entry.setCrc(crc.getValue());
            return new EntryDataPair(entry, data);
        }
    }

    private static EntryDataPair processEntry(ZipEntry entry, byte[] data)
            throws ClassPathValidator.ClassNotLoadedException {
        // Copy all non-.class files to the output jar.
        if (entry.isDirectory() || !entry.getName().endsWith(CLASS_FILE_SUFFIX)) {
            return new EntryDataPair(entry, data);
        }

        ClassReader reader = new ClassReader(data);
        if (sShouldCheckClassPath) {
            sValidator.validateClassPathsAndOutput(reader, sDirectClassPathClassLoader,
                    sFullClassPathClassLoader, sFullClassPathJarPaths, sIsPrebuilt, sVerbose,
                    sMissingClassesAllowlist);
        }

        ClassWriter writer = new ClassWriter(reader, 0);
        ClassVisitor chain = writer;
        /* DEBUGGING:
         To see objectweb.asm code that will generate bytecode for a given class:

         java -cp
         "third_party/android_deps/libs/org_ow2_asm_asm/asm-7.0.jar:third_party/android_deps/libs/org_ow2_asm_asm_util/asm-util-7.0.jar:out/Debug/lib.java/jar_containing_yourclass.jar"
         org.objectweb.asm.util.ASMifier org.package.YourClassName

         See this pdf for more details: https://asm.ow2.io/asm4-guide.pdf

         To see the bytecode for a specific class, uncomment this code with your class name:

        if (entry.getName().contains("YOUR_CLASS_NAME")) {
          chain = new TraceClassVisitor(chain, new PrintWriter(System.out));
        }
        */
        if (sShouldUseThreadAnnotations) {
            chain = new ThreadAssertionClassAdapter(chain);
        }
        chain = org.brave.bytecode.BraveClassAdapter.createAdapter(chain);
        reader.accept(chain, 0);
        byte[] patchedByteCode = writer.toByteArray();
        return EntryDataPair.create(entry.getName(), patchedByteCode);
    }

    private static void process(String inputJarPath, String outputJarPath)
            throws ClassPathValidator.ClassNotLoadedException, ExecutionException,
                   InterruptedException {
        String tempJarPath = outputJarPath + TEMPORARY_FILE_SUFFIX;
        ExecutorService executorService =
                Executors.newFixedThreadPool(Runtime.getRuntime().availableProcessors());
        try (ZipInputStream inputStream = new ZipInputStream(
                     new BufferedInputStream(new FileInputStream(inputJarPath)));
                ZipOutputStream tempStream = new ZipOutputStream(
                        new BufferedOutputStream(new FileOutputStream(tempJarPath)))) {
            List<Future<EntryDataPair>> list = new ArrayList<>();
            while (true) {
                ZipEntry entry = inputStream.getNextEntry();
                if (entry == null) {
                    break;
                }
                byte[] data = readAllBytes(inputStream);
                list.add(executorService.submit(() -> processEntry(entry, data)));
            }
            executorService.shutdown(); // This is essential in order to avoid waiting infinitely.
            // Write the zip file entries in order to preserve determinism.
            for (Future<EntryDataPair> futurePair : list) {
                EntryDataPair pair = futurePair.get();
                tempStream.putNextEntry(pair.mEntry);
                tempStream.write(pair.mData);
                tempStream.closeEntry();
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        try {
            Path src = Paths.get(tempJarPath);
            Path dest = Paths.get(outputJarPath);
            Files.move(src, dest, StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException ioException) {
            throw new RuntimeException(ioException);
        }

        if (sValidator.hasErrors()) {
            System.err.println("Direct classpath is incomplete. To fix, add deps on the "
                    + "GN target(s) that provide:");
            for (Map.Entry<String, Map<String, Set<String>>> entry :
                    sValidator.getErrors().entrySet()) {
                printValidationError(System.err, entry.getKey(), entry.getValue());
            }
            System.exit(1);
        }
    }

    private static void printValidationError(
            PrintStream out, String jarName, Map<String, Set<String>> missingClasses) {
        out.print(" * ");
        out.println(jarName);
        int i = 0;
        final int numErrorsPerJar = 2;
        // The list of missing classes is non-exhaustive because each class that fails to validate
        // reports only the first missing class.
        for (Map.Entry<String, Set<String>> entry : missingClasses.entrySet()) {
            String missingClass = entry.getKey();
            Set<String> filesThatNeededIt = entry.getValue();
            out.print("     * ");
            if (i == numErrorsPerJar) {
                out.print(String.format("And %d more...", missingClasses.size() - numErrorsPerJar));
                break;
            }
            out.print(missingClass.replace('/', '.'));
            out.print(" (needed by ");
            out.print(filesThatNeededIt.iterator().next().replace('/', '.'));
            if (filesThatNeededIt.size() > 1) {
                out.print(String.format(" and %d more", filesThatNeededIt.size() - 1));
            }
            out.println(")");
            i++;
        }
    }

    private static byte[] readAllBytes(InputStream inputStream) throws IOException {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        int numRead = 0;
        byte[] data = new byte[BUFFER_SIZE];
        while ((numRead = inputStream.read(data, 0, data.length)) != -1) {
            buffer.write(data, 0, numRead);
        }
        return buffer.toByteArray();
    }

    /**
     * Loads a list of jars and returns a ClassLoader capable of loading all classes found in the
     * given jars.
     */
    static ClassLoader loadJars(Collection<String> paths) {
        URL[] jarUrls = new URL[paths.size()];
        int i = 0;
        for (String path : paths) {
            try {
                jarUrls[i++] = new File(path).toURI().toURL();
            } catch (MalformedURLException e) {
                throw new RuntimeException(e);
            }
        }
        return new URLClassLoader(jarUrls);
    }

    /**
     * Extracts a length-encoded list of strings from the arguments, and adds them to |out|. Returns
     * the new "next index" to be processed.
     */
    private static int parseListArgument(String[] args, int index, Collection<String> out) {
        int argLength = Integer.parseInt(args[index++]);
        out.addAll(Arrays.asList(Arrays.copyOfRange(args, index, index + argLength)));
        return index + argLength;
    }

    public static void main(String[] args) throws ClassPathValidator.ClassNotLoadedException,
                                                  ExecutionException, InterruptedException {
        // Invoke this script using //build/android/gyp/bytecode_processor.py
        int currIndex = 0;
        String inputJarPath = args[currIndex++];
        String outputJarPath = args[currIndex++];
        sVerbose = args[currIndex++].equals("--verbose");
        sIsPrebuilt = args[currIndex++].equals("--is-prebuilt");
        sShouldUseThreadAnnotations = args[currIndex++].equals("--enable-thread-annotations");
        sShouldCheckClassPath = args[currIndex++].equals("--enable-check-class-path");

        sMissingClassesAllowlist = new HashSet<>();
        currIndex = parseListArgument(args, currIndex, sMissingClassesAllowlist);

        ArrayList<String> sdkJarPaths = new ArrayList<>();
        currIndex = parseListArgument(args, currIndex, sdkJarPaths);

        ArrayList<String> directClassPathJarPaths = new ArrayList<>();
        directClassPathJarPaths.add(inputJarPath);
        directClassPathJarPaths.addAll(sdkJarPaths);
        currIndex = parseListArgument(args, currIndex, directClassPathJarPaths);
        sDirectClassPathClassLoader = loadJars(directClassPathJarPaths);

        // Load all jars that are on the classpath for the input jar for analyzing class
        // hierarchy.
        sFullClassPathJarPaths = new HashSet<>();
        sFullClassPathJarPaths.clear();
        sFullClassPathJarPaths.add(inputJarPath);
        sFullClassPathJarPaths.addAll(sdkJarPaths);
        sFullClassPathJarPaths.addAll(
                Arrays.asList(Arrays.copyOfRange(args, currIndex, args.length)));

        sFullClassPathClassLoader = loadJars(sFullClassPathJarPaths);
        sFullClassPathJarPaths.removeAll(directClassPathJarPaths);

        sValidator = new ClassPathValidator();
        process(inputJarPath, outputJarPath);
    }
}
