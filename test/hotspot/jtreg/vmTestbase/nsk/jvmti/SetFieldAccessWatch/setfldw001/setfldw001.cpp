/*
 * Copyright (c) 2003, 2024, Oracle and/or its affiliates. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "jvmti.h"
#include "agent_common.hpp"
#include "JVMTITools.h"

extern "C" {


#define PASSED  0
#define STATUS_FAILED  2

typedef struct {
    const char *klass;
    const char *name;
    const char *sig;
    int stat;
    jfieldID fid;
    jfieldID thrown_fid;
} field;

static jvmtiEnv *jvmti;
static jvmtiEventCallbacks callbacks;
static jvmtiCapabilities caps;
static jint result = PASSED;
static field fields[] = {
    { "nsk/jvmti/SetFieldAccessWatch/setfldw001", "fld0", "I", 0, nullptr, nullptr },
    { "nsk/jvmti/SetFieldAccessWatch/setfldw001", "fld1", "I", 1, nullptr, nullptr },
    { "nsk/jvmti/SetFieldAccessWatch/setfldw001", "fld2",
      "Lnsk/jvmti/SetFieldAccessWatch/setfldw001a;", 0, nullptr, nullptr },
    { "nsk/jvmti/SetFieldAccessWatch/setfldw001a", "fld3", "[I", 0, nullptr, nullptr },
    { "nsk/jvmti/SetFieldAccessWatch/setfldw001b", "fld4", "F", 0, nullptr, nullptr },
};

void setWatch(JNIEnv *env, jint ind) {
    jvmtiError err;
    jclass cls;
    field fld = fields[ind];

    cls = env->FindClass(fld.klass);
    if (cls == nullptr) {
        printf("Cannot find class \"%s\"\n", fld.klass);
        result = STATUS_FAILED;
        return;
    }
    if (fld.fid == nullptr) {
        if (fld.stat) {
            fields[ind].fid = env->GetStaticFieldID(cls, fld.name, fld.sig);
        } else {
            fields[ind].fid = env->GetFieldID(cls, fld.name, fld.sig);
        }
    }

    err = jvmti->SetFieldAccessWatch(cls, fields[ind].fid);
    if (err == JVMTI_ERROR_MUST_POSSESS_CAPABILITY &&
            !caps.can_generate_field_access_events) {
        /* Ok, it's expected */
    } else if (err != JVMTI_ERROR_NONE) {
        printf("(SetFieldAccessWatch#%d) unexpected error: %s (%d)\n",
               ind, TranslateError(err), err);
        result = STATUS_FAILED;
    }
}

void JNICALL FieldAccess(jvmtiEnv *jvmti_env, JNIEnv *env,
        jthread thr, jmethodID method,
        jlocation location, jclass field_klass, jobject obj, jfieldID field) {

    char *fld_name = nullptr;
    jint fld_ind = 0;
    size_t len = 0;
    jvmtiError err = jvmti_env->GetFieldName(field_klass, field,
                                                &fld_name, nullptr, nullptr);
    if (err != JVMTI_ERROR_NONE) {
        printf("Error in GetFieldName: %s (%d)\n", TranslateError(err), err);
        result = STATUS_FAILED;
        return;
    }
    if (fld_name == nullptr) {
        printf("GetFieldName returned null field name\n");
        result = STATUS_FAILED;
        return;
    }
    len = strlen(fld_name);
    /* All field names have the same length and end with a digit. */
    if (len != strlen(fields[0].name) || !isdigit(fld_name[len - 1])) {
        printf("GetFieldName returned unexpected field name: %s\n", fld_name);
        result = STATUS_FAILED;
        return;
    }
    fld_ind = (int)(fld_name[len - 1] - '0'); /* last digit is index in the array */
    fields[fld_ind].thrown_fid = field;
    jvmti_env->Deallocate((unsigned char*) fld_name);
}

#ifdef STATIC_BUILD
JNIEXPORT jint JNICALL Agent_OnLoad_setfldw001(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNICALL Agent_OnAttach_setfldw001(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNI_OnLoad_setfldw001(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif
jint  Agent_Initialize(JavaVM *jvm, char *options, void *reserved) {
    jint res;
    jvmtiError err;

    res = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_1);
    if (res != JNI_OK || jvmti == nullptr) {
        printf("Wrong result of a valid call to GetEnv !\n");
        return JNI_ERR;
    }

    err = jvmti->GetPotentialCapabilities(&caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("(GetPotentialCapabilities) unexpected error: %s (%d)\n",
               TranslateError(err), err);
        return JNI_ERR;
    }

    err = jvmti->AddCapabilities(&caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("(AddCapabilities) unexpected error: %s (%d)\n",
               TranslateError(err), err);
        return JNI_ERR;
    }

    err = jvmti->GetCapabilities(&caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("(GetCapabilities) unexpected error: %s (%d)\n",
               TranslateError(err), err);
        return JNI_ERR;
    }

    if (caps.can_generate_field_access_events) {
        callbacks.FieldAccess = &FieldAccess;
        err = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
        if (err != JVMTI_ERROR_NONE) {
            printf("(SetEventCallbacks) unexpected error: %s (%d)\n",
                   TranslateError(err), err);
            return JNI_ERR;
        }

        err = jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                JVMTI_EVENT_FIELD_ACCESS, nullptr);
        if (err != JVMTI_ERROR_NONE) {
            printf("Failed to enable JVMTI_EVENT_FIELD_ACCESS: %s (%d)\n",
                   TranslateError(err), err);
            return JNI_ERR;
        }
    } else {
        printf("Warning: FieldAccess watch is not implemented\n");
    }

    return JNI_OK;
}

JNIEXPORT void JNICALL
Java_nsk_jvmti_SetFieldAccessWatch_setfldw001_setWatch(JNIEnv *env,
        jclass cls, jint fld_ind) {
    setWatch(env, fld_ind);
}

JNIEXPORT void JNICALL
Java_nsk_jvmti_SetFieldAccessWatch_setfldw001_touchfld0(JNIEnv *env,
        jobject obj) {
    jint val;

    setWatch(env, (jint)0);
    val = env->GetIntField(obj, fields[0].fid);
}

JNIEXPORT void JNICALL
Java_nsk_jvmti_SetFieldAccessWatch_setfldw001_check(JNIEnv *env,
        jclass cls, jint fld_ind, jboolean flag) {
    jfieldID thrown_fid = fields[fld_ind].thrown_fid;

    if (caps.can_generate_field_access_events) {
        if (flag == JNI_FALSE && thrown_fid != nullptr) {
            result = STATUS_FAILED;
            printf("(Field %d) FIELD_ACCESS event without access watch set\n",
                   fld_ind);
        } else if (flag == JNI_TRUE && thrown_fid != fields[fld_ind].fid) {
            result = STATUS_FAILED;
            printf("(Field %d) thrown field ID expected: 0x%p, got: 0x%p\n",
                   fld_ind, fields[fld_ind].fid, thrown_fid);
        }
    }
}

JNIEXPORT jint JNICALL
Java_nsk_jvmti_SetFieldAccessWatch_setfldw001_getRes(JNIEnv *env, jclass cls) {
    return result;
}

}
