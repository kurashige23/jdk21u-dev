/*
 * Copyright (c) 2004, 2024, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <stdlib.h>
#include <string.h>
#include "jni_tools.h"
#include "agent_common.hpp"
#include "jvmti_tools.h"

extern "C" {

/* ========================================================================== */

/* scaffold objects */
static jlong timeout = 0;

/* test objects */
static jobject testedClassLoader = nullptr;
static jclass testedClass = nullptr;
static jfieldID testedFieldID = nullptr;

static const char *CLASS_SIG[] = {
    "Lnsk/jvmti/GetLoadedClasses/loadedclss002;",
    "[Lnsk/jvmti/GetLoadedClasses/loadedclss002;",
    "Ljava/lang/Object;",
    "[Ljava/lang/Object;",
    "[Z", "[B", "[C", "[I", "[S", "[J", "[F", "[D"
};

static const char *PRIM_SIG[] = {
    "Z", "B", "C", "I", "S", "J", "F", "D"
};

/* ========================================================================== */

static int lookup(jvmtiEnv* jvmti,
        jint classCount, jclass *classes, const char *exp_sig) {
    char *signature, *generic;
    int found = NSK_FALSE;
    jint i;

    for (i = 0; i < classCount && !found; i++) {
        if (!NSK_JVMTI_VERIFY(jvmti->GetClassSignature(classes[i], &signature, &generic)))
            break;

        if (signature != nullptr && strcmp(signature, exp_sig) == 0) {
            NSK_DISPLAY1("Expected class found: %s\n", exp_sig);
            found = NSK_TRUE;
        }

        if (signature != nullptr)
            jvmti->Deallocate((unsigned char*)signature);

        if (generic != nullptr)
            jvmti->Deallocate((unsigned char*)generic);
    }

    return found;
}

/* ========================================================================== */

/** Agent algorithm. */
static void JNICALL
agentProc(jvmtiEnv* jvmti, JNIEnv* jni, void* arg) {
    jclass *classes;
    jint classCount;
    size_t i;

    if (!nsk_jvmti_waitForSync(timeout))
        return;

    if (!NSK_JVMTI_VERIFY(jvmti->GetLoadedClasses(&classCount, &classes))) {
        nsk_jvmti_setFailStatus();
        return;
    }

    if (!NSK_VERIFY(classCount != 0)) {
        nsk_jvmti_setFailStatus();
        return;
    }

    if (!NSK_VERIFY(classes != nullptr)) {
        nsk_jvmti_setFailStatus();
        return;
    }

    for (i = 0; i < sizeof(CLASS_SIG)/sizeof(char*); i++) {
        if (!lookup(jvmti, classCount, classes, CLASS_SIG[i])) {
            NSK_COMPLAIN1("Cannot find class: %s\n", CLASS_SIG[i]);
            nsk_jvmti_setFailStatus();
            return;
        }
    }

    for (i = 0; i < sizeof(PRIM_SIG)/sizeof(char*); i++) {
        if (lookup(jvmti, classCount, classes, PRIM_SIG[i])) {
            NSK_COMPLAIN1("Primitive class found: %s\n", PRIM_SIG[i]);
            nsk_jvmti_setFailStatus();
            return;
        }
    }

    if (classes != nullptr)
        jvmti->Deallocate((unsigned char*)classes);

    if (!nsk_jvmti_resumeSync())
        return;
}

/* ========================================================================== */

/** Agent library initialization. */
#ifdef STATIC_BUILD
JNIEXPORT jint JNICALL Agent_OnLoad_loadedclss002(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNICALL Agent_OnAttach_loadedclss002(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNI_OnLoad_loadedclss002(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif
jint Agent_Initialize(JavaVM *jvm, char *options, void *reserved) {
    jvmtiEnv* jvmti = nullptr;

    NSK_DISPLAY0("Agent_OnLoad\n");

    if (!NSK_VERIFY(nsk_jvmti_parseOptions(options)))
        return JNI_ERR;

    timeout = nsk_jvmti_getWaitTime() * 60 * 1000;

    if (!NSK_VERIFY((jvmti =
            nsk_jvmti_createJVMTIEnv(jvm, reserved)) != nullptr))
        return JNI_ERR;

    if (!NSK_VERIFY(nsk_jvmti_setAgentProc(agentProc, nullptr)))
        return JNI_ERR;

    return JNI_OK;
}

/* ========================================================================== */

}
