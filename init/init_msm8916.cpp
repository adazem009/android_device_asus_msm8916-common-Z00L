/*
   Copyright (c) 2013, The Linux Foundation. All rights reserved.
   Copyright (C) 2016 The CyanogenMod Project.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include <android-base/properties.h>
#include "vendor_init.h"
#include "property_service.h"

using android::base::GetProperty;
using android::init::property_set;

char const *device;
char const *family;
char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;
char const *buildnumber;
char const *builddate;

void check_device()
{
    int PRJ_ID, PRJ_SKU, PRJ_HD;
    struct sysinfo sys;
    FILE *fp;

    fp = fopen("/proc/apid", "r");
    fscanf(fp, "%d", &PRJ_ID);
    fclose(fp);

    fp = fopen("/proc/aprf", "r");
    fscanf(fp, "%d", &PRJ_SKU);
    fclose(fp);

    fp = fopen("/proc/aphd", "r");
    fscanf(fp, "%d", &PRJ_HD);
    fclose(fp);

    sysinfo(&sys);

    if (PRJ_HD == 1) {
        family = "Z00L";
        if (PRJ_ID == 0) {
            if (PRJ_SKU == 3) {
                device = "Z00W"; // ZE550KG
                buildnumber = "2179";
                builddate = "20170803";
            } else {
                device = "Z00L"; // ZE550KL
                buildnumber = "2179";
                builddate = "20170803";
            }
        } else if (PRJ_ID == 1) {
            device = "Z00M"; // ZE600KL
            buildnumber = "2171";
            builddate = "20170719";
        }

        // from - phone-xhdpi-2048-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heapminfree = "512k";
    } else if (PRJ_HD == 0) {
        family = "Z00T";
        if (PRJ_ID == 0) {
            device = "Z00T"; // ZE551KL
            buildnumber = "2056";
            builddate = "20170224";
        } else if (PRJ_ID == 1) {
            device = "Z011"; // ZE601KL
            buildnumber = "2170";
            builddate = "20170719";
        } else if (PRJ_ID == 2) {
            device = "Z00C"; // ZX550KL
            buildnumber = "2056";
            builddate = "20170224";
        } else if (PRJ_ID == 3) {
            device = "Z00U"; // ZD551KL
            buildnumber = "2214";
            builddate = "20171110";
        }

        if (sys.totalram > 2048ull * 1024 * 1024) {
            // from - phone-xxhdpi-3072-dalvik-heap.mk
            heapstartsize = "8m";
            heapgrowthlimit = "288m";
            heapsize = "768m";
            heapminfree = "512k";
        } else if (sys.totalram > 1024ull * 1024 * 1024) {
            // from - phone-xxhdpi-2048-dalvik-heap.mk
            heapstartsize = "16m";
            heapgrowthlimit = "192m";
            heapsize = "512m";
            heapminfree = "2m";
        } else {
            // from - phone-xhdpi-1024-dalvik-heap.mk
            heapstartsize = "8m";
            heapgrowthlimit = "96m";
            heapsize = "256m";
            heapminfree = "2m";
        }
    }
}

bool is_target_8916()
{
    int fd;
    int soc_id = -1;
    char buf[10] = { 0 };

    if (access("/sys/devices/soc0/soc_id", F_OK) == 0)
        fd = open("/sys/devices/soc0/soc_id", O_RDONLY);
    else
        fd = open("/sys/devices/system/soc/soc0/id", O_RDONLY);

    if (fd >= 0 && read(fd, buf, sizeof(buf) - 1) != -1)
        soc_id = atoi(buf);

    close(fd);
    return soc_id == 206 || (soc_id >= 247 && soc_id <= 250);
}

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_override_triple(char const product_prop[], char const system_prop[], char const vendor_prop[], char const value[])
{
    property_override(product_prop, value);
    property_override(system_prop, value);
    property_override(vendor_prop, value);
}

void vendor_load_properties()
{
    char b_description[PROP_VALUE_MAX], b_fingerprint[PROP_VALUE_MAX];
    char p_carrier[PROP_VALUE_MAX], p_device[PROP_VALUE_MAX], p_model[PROP_VALUE_MAX];

    std::string platform = GetProperty("ro.board.platform", "");
    if (platform != ANDROID_TARGET)
        return;

    // Init a dummy BT MAC address, will be overwritten later
    property_set("ro.boot.btmacaddr", "00:00:00:00:00:00");
    property_override("ro.debuggable", "0");
    property_override_triple("ro.build.type", "ro.system.build.type", "ro.vendor.build.type", "user");	
    property_override_triple("ro.build.tags", "ro.system.build.tags", "ro.vendor.build.tags", "release-keys");
    check_device();

    sprintf(b_description, "Z00L-user 6.0.1 MMB29P WW_user_21.40.1220.2196_20180308 release-keys", family, buildnumber, builddate);
    sprintf(b_fingerprint, "asus/WW_Z00L/ASUS_Z00L_63:6.0.1/MMB29P/WW_user_21.40.1220.2196_20180308:user/release-keys", device, device, buildnumber, builddate);
    sprintf(p_model, "ASUS_%sD", device);
    sprintf(p_device, "ASUS_%s", device);
    sprintf(p_carrier, "US-ASUS_%s-WW_%s", device, device);

    property_override_triple("ro.build.description", "ro.system.build.description", "ro.vendor.description", b_description);
    property_override("ro.product.carrier", p_carrier);
    property_override_triple("ro.build.fingerprint", "ro.system.build.fingerprint", "ro.vendor.build.fingerprint", b_fingerprint);
    property_override_triple("ro.product.device", "ro.product.system.device", "ro.product.vendor.device", p_device);
    property_override_triple("ro.product.model", "ro.product.system.model", "ro.product.vendor.model", p_model);

    property_set("dalvik.vm.heapstartsize", heapstartsize);
    property_set("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
    property_set("dalvik.vm.heapsize", heapsize);
    property_set("dalvik.vm.heaptargetutilization", "0.75");
    property_set("dalvik.vm.heapminfree", heapminfree);
    property_set("dalvik.vm.heapmaxfree", "8m");

    if (is_target_8916()) {
        property_set("ro.opengles.version", "196608");
    } else {
        property_set("ro.opengles.version", "196610");
    }	
}
