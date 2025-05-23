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

#include <string.h>
#include "jvmti.h"
#include "agent_common.hpp"
#include "jni_tools.h"
#include "jvmti_tools.h"

extern "C" {


/* min and max values defined in jvmtiParamTypes */
#define PARAM_TYPE_MIN_VALUE 101
#define PARAM_TYPE_MAX_VALUE 117

/* min and max values defined in jvmtiParamKind */
#define PARAM_KIND_MIN_VALUE  91
#define PARAM_KIND_MAX_VALUE  97

#define NAME_PREFIX "com.sun.hotspot"

/* ============================================================================= */

static jlong timeout = 0;

/* ============================================================================= */

/** Check extension functions. */
static int checkExtensions(jvmtiEnv* jvmti, const char phase[]) {
    int success = NSK_TRUE;
    jint extCount = 0;
    jvmtiExtensionEventInfo* extList = nullptr;
    int i;

    NSK_DISPLAY0("Get extension events list\n");
    if (!NSK_JVMTI_VERIFY(jvmti->GetExtensionEvents(&extCount, &extList))) {
        return NSK_FALSE;
    }
    NSK_DISPLAY1("  ... got count: %d\n", (int)extCount);
    NSK_DISPLAY1("  ... got list:  0x%p\n", (void*)extList);

    if (extCount > 0) {
        if (extList == nullptr) {
            NSK_COMPLAIN3("In %s phase GetExtensionEvents() returned null pointer:\n"
                          "#   extensions pointer: 0x%p\n"
                          "#   extensions count:   %d\n",
                            phase, (void*)extList, (int)extCount);
            return NSK_FALSE;
        }

        NSK_DISPLAY1("Check each extension events: %d events\n", (int)extCount);
        for (i = 0; i < extCount; i++) {
            int j;

            NSK_DISPLAY1("  event #%d:\n", i);
            NSK_DISPLAY1("    event_index: %d\n", (int)extList[i].extension_event_index);
            NSK_DISPLAY1("    id:          \"%s\"\n", nsk_null_string(extList[i].id));
            NSK_DISPLAY1("    short_desc:  \"%s\"\n", nsk_null_string(extList[i].short_description));
            NSK_DISPLAY1("    param_count: %d\n", (int)extList[i].param_count);
            NSK_DISPLAY1("    params:      0x%p\n", (void*)extList[i].params);

            if (extList[i].params != nullptr) {
                for (j = 0; j < extList[i].param_count; j++) {
                    NSK_DISPLAY1("      param #%d:\n", j);
                    NSK_DISPLAY1("        name:      \"%s\"\n",
                                            nsk_null_string(extList[i].params[j].name));
                    NSK_DISPLAY1("        kind:      %d\n",
                                            (int)extList[i].params[j].kind);
                    NSK_DISPLAY1("        base_type: %d\n",
                                            (int)extList[i].params[j].base_type);
                    NSK_DISPLAY1("        null_ok:   %d\n",
                                            (int)extList[i].params[j].null_ok);
                }
            }

            if (extList[i].id == nullptr
                    || extList[i].short_description == nullptr
                    || (extList[i].params == nullptr && extList[i].param_count > 0)) {
                NSK_COMPLAIN9("In %s phase GetExtensionEvents() returned event #%d with null attribute(s):\n"
                              "#   event_index: %d\n"
                              "#   id:          0x%p (%s)\n"
                              "#   short_desc:  0x%p (%s)\n"
                              "#   param_count: %d\n"
                              "#   params:      0x%p\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                (void*)extList[i].id, nsk_null_string(extList[i].id),
                                (void*)extList[i].short_description, nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count, (void*)extList[i].params);
                success = NSK_FALSE;
            }

            if (extList[i].id != nullptr && strlen(extList[i].id) <= 0) {
                NSK_COMPLAIN6("In %s phase GetExtensionEvents() returned event #%d with empty id:\n"
                              "#   event_index: %d\n"
                              "#   id:          \"%s\"\n"
                              "#   short_desc:  \"%s\"\n"
                              "#   param_count: %d\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                nsk_null_string(extList[i].id),
                                nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count);
                success = NSK_FALSE;
            } else if (strstr(extList[i].id, NAME_PREFIX) == nullptr) {
                NSK_COMPLAIN6("In %s phase GetExtensionEvents() returned event #%d with unexpected id:\n"
                              "#   event_index: %d\n"
                              "#   id:          \"%s\"\n"
                              "#   short_desc:  \"%s\"\n"
                              "#   param_count: %d\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                nsk_null_string(extList[i].id),
                                nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count);
                success = NSK_FALSE;
            }

            if (extList[i].short_description != nullptr && strlen(extList[i].short_description) <= 0) {
                NSK_COMPLAIN6("In %s phase GetExtensionEvents() returned event #%d with empty desc:\n"
                              "#   event_index: %d\n"
                              "#   id:          \"%s\"\n"
                              "#   short_desc:  \"%s\"\n"
                              "#   param_count: %d\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                nsk_null_string(extList[i].id),
                                nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count);
                success = NSK_FALSE;
            }

            if (extList[i].param_count > 0 && extList[i].params != nullptr) {
                for (j = 0; j < extList[i].param_count; j++) {
                    if (extList[i].params[j].name == nullptr
                            || strlen(extList[i].params[j].name) <= 0) {
                        NSK_COMPLAIN9("In %s phase GetExtensionEvents() returned event #%d with empty desc:\n"
                                      "#   event_index: %d\n"
                                      "#   id:          \"%s\"\n"
                                      "#   short_desc:  \"%s\"\n"
                                      "#   param_count: %d\n"
                                      "#     param #%d: \n"
                                      "#       name:    0x%p (%s)\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                nsk_null_string(extList[i].id),
                                nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count,
                                j,
                                (void*)extList[i].params[j].name,
                                nsk_null_string(extList[i].params[j].name));
                        success = NSK_FALSE;
                    }

                    if (extList[i].params[j].kind < PARAM_KIND_MIN_VALUE
                            || extList[i].params[j].kind > PARAM_KIND_MAX_VALUE) {
                        NSK_COMPLAIN9("In %s phase GetExtensionEvents() returned event #%d with incorrect parameter kind:\n"
                                      "#   event_index: %d\n"
                                      "#   id:          \"%s\"\n"
                                      "#   short_desc:  \"%s\"\n"
                                      "#   param_count: %d\n"
                                      "#     param #%d: \n"
                                      "#       name:    0x%p (%s)\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                nsk_null_string(extList[i].id),
                                nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count,
                                j,
                                (void*)extList[i].params[j].name,
                                nsk_null_string(extList[i].params[j].name));
                        success = NSK_FALSE;
                    }

                    if (extList[i].params[j].base_type < PARAM_TYPE_MIN_VALUE
                            || extList[i].params[j].base_type > PARAM_TYPE_MAX_VALUE) {
                        NSK_COMPLAIN9("In %s phase GetExtensionEvents() returned event #%d with incorrect parameter type:\n"
                                      "#   event_index: %d\n"
                                      "#   id:          \"%s\"\n"
                                      "#   short_desc:  \"%s\"\n"
                                      "#   param_count: %d\n"
                                      "#     param #%d: \n"
                                      "#       name:    0x%p (%s)\n",
                                phase, i,
                                (int)extList[i].extension_event_index,
                                nsk_null_string(extList[i].id),
                                nsk_null_string(extList[i].short_description),
                                (int)extList[i].param_count,
                                j,
                                (void*)extList[i].params[j].name,
                                nsk_null_string(extList[i].params[j].name));
                        success = NSK_FALSE;
                    }
                }
            }
        }
    }

    NSK_DISPLAY1("Deallocate extension events list: 0x%p\n", (void*)extList);
    if (!NSK_JVMTI_VERIFY(jvmti->Deallocate((unsigned char*)extList))) {
        return NSK_FALSE;
    }
    NSK_DISPLAY0("  ... deallocated\n");

    return success;
}

/* ============================================================================= */

/** Agent algorithm. */
static void JNICALL
agentProc(jvmtiEnv* jvmti, JNIEnv* jni, void* arg) {
    NSK_DISPLAY0("Wait for debugee class ready\n");
    if (!NSK_VERIFY(nsk_jvmti_waitForSync(timeout)))
        return;

    NSK_DISPLAY0(">>> Testcase #2: Check extension events in live phase\n");
    {
        if (!checkExtensions(jvmti, "live")) {
            nsk_jvmti_setFailStatus();
        }
    }

    NSK_DISPLAY0("Let debugee to finish\n");
    if (!NSK_VERIFY(nsk_jvmti_resumeSync()))
        return;
}

/* ============================================================================= */

/** Agent library initialization. */
#ifdef STATIC_BUILD
JNIEXPORT jint JNICALL Agent_OnLoad_extevents001(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNICALL Agent_OnAttach_extevents001(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNI_OnLoad_extevents001(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif
jint Agent_Initialize(JavaVM *jvm, char *options, void *reserved) {
    jvmtiEnv* jvmti = nullptr;

    if (!NSK_VERIFY(nsk_jvmti_parseOptions(options)))
        return JNI_ERR;

    timeout = nsk_jvmti_getWaitTime() * 60 * 1000;

    if (!NSK_VERIFY((jvmti =
            nsk_jvmti_createJVMTIEnv(jvm, reserved)) != nullptr))
        return JNI_ERR;

    NSK_DISPLAY0(">>> Testcase #1: Check extension events in OnLoad phase\n");
    {
        if (!checkExtensions(jvmti, "OnLoad")) {
            nsk_jvmti_setFailStatus();
        }
    }

    if (!NSK_VERIFY(nsk_jvmti_setAgentProc(agentProc, nullptr)))
        return JNI_ERR;

    return JNI_OK;
}

/* ============================================================================= */

}
