/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import static org.objectweb.asm.Opcodes.ACC_PRIVATE;
import static org.objectweb.asm.Opcodes.ACC_PUBLIC;
import static org.objectweb.asm.Opcodes.ASM5;

import java.util.HashMap;
import java.util.Map;
import java.util.ArrayList;

import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;

class BraveClassVisitor extends ClassVisitor {

    class Method {
        public int access;
        public String name;
        public String desc;
        public String signature;
        public String[] exceptions;

        public Method(final int access, final String name, String desc,
                      String signature, String[] exceptions) {
            this.access = access;
            this.name = name;
            this.desc = desc;
            this.signature = signature;
            this.exceptions = exceptions;
        }

        public void makePublic() {
            access &= ~ACC_PRIVATE;
            access |= ACC_PUBLIC;
        }
    }

    protected String mName = "";
    protected String mSuperName = "";

    private Map<String, String> mSuperNames = new HashMap<String, String>();
    private Map<String, ArrayList<String>> mDeleteMethods =
            new HashMap<String, ArrayList<String>>();
    private Map<String, ArrayList<String>> mMakePublicMethods =
            new HashMap<String, ArrayList<String>>();

    public BraveClassVisitor(ClassVisitor visitor) {
        super(ASM5, visitor);
    }

    protected void changeSuperName(String className, String superName) {
        mSuperNames.put(className, superName);
    }

    private boolean shouldDeleteMethod(String methodName) {
        for(Map.Entry<String, ArrayList<String>> entry :
                mDeleteMethods.entrySet()) {
            String className = entry.getKey();
            ArrayList<String> methodNames = entry.getValue();
            return mName.contains(className) &&
                   methodNames.contains(methodName);
        }

        return false;
    }

    protected void deleteMethod(String className, String methodName) {
        ArrayList methods = mDeleteMethods.get(className);
        if (methods == null) {
            methods = new ArrayList<String>();
            mDeleteMethods.put(className, methods);
        }
        methods.add(methodName);
    }

    private boolean shouldMakePublicMethod(String methodName) {
        for(Map.Entry<String, ArrayList<String>> entry :
                mMakePublicMethods.entrySet()) {
            String className = entry.getKey();
            ArrayList<String> methodNames = entry.getValue();
            return mName.contains(className) &&
                   methodNames.contains(methodName);
        }

        return false;
    }

    protected void makePublicMethod(String className, String methodName) {
        ArrayList methods = mMakePublicMethods.get(className);
        if (methods == null) {
            methods = new ArrayList<String>();
            mMakePublicMethods.put(className, methods);
        }
        methods.add(methodName);
    }

    @Override
    public void visit(int version,
                      int access,
                      String name,
                      String signature,
                      String superName,
                      String[] interfaces) {
        mName = name;
        if (mSuperNames.containsKey(name)) {
            superName = mSuperNames.get(name);
            System.out.println("change superclass of " + name + " to " + superName);
        }

        visitImpl(version, access, name, signature, superName, interfaces);
    }

    protected void visitImpl(int version,
                             int access,
                             String name,
                             String signature,
                             String superName,
                             String[] interfaces) {
        super.visit(version, access, name, signature, superName, interfaces);
    }

    @Override
    public MethodVisitor visitMethod(final int access,
                                     final String name,
                                     String desc,
                                     String signature,
                                     String[] exceptions) {
        Method method = new Method(access, name, desc, signature, exceptions);
        if (shouldDeleteMethod(name)) {
            System.out.println("delete " + name + " from " + mName);
            return null;
        }

        if (shouldMakePublicMethod(name)) {
            System.out.println("make " + name + " public in " + mName);
            method.makePublic();
        }

        return visitMethodImpl(method);
    }

    protected MethodVisitor visitMethodImpl(Method method) {
        return super.visitMethod(method.access,
                                 method.name,
                                 method.desc,
                                 method.signature,
                                 method.exceptions);
    }
}
