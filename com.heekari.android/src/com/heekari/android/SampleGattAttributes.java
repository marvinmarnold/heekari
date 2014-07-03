/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.heekari.android;

import java.util.HashMap;

/**
 * This class includes a small subset of standard GATT attributes for demonstration purposes.
 */
public class SampleGattAttributes {
    private static HashMap<String, String> attributes = new HashMap();
    public static String HEART_RATE_MEASUREMENT = "00002a37-0000-1000-8000-00805f9b34fb";
    public static String CLIENT_CHARACTERISTIC_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";
    public static String SWITCH_UUID = "5F55AEF5-09D6-48A5-B44B-E41D7DF55743";

    public static String HEART_RATE_SERVICE = "0000180d-0000-1000-8000-00805f9b34fb";
    
    // Security tag
    public static String ALERT_SERVICE = "00001802-0000-1000-8000-00805f9b34fb";
    public static String ALERT_LEVEL = "00002a06-0000-1000-8000-00805f9b34fb";
    
    
    public static String LINK_LOSS_SERVICE = "00001803-0000-1000-8000-00805f9b34fb";
    
    public static String TX_SERVICE = "00001804-0000-1000-8000-00805f9b34fb";
    public static String TX_POWER_LEVEL = "00002a07-0000-1000-8000-00805f9b34fb";
    
    static {
        // Sample Services.
        attributes.put(HEART_RATE_SERVICE, "Heart Rate Service");
        attributes.put("0000180a-0000-1000-8000-00805f9b34fb", "Device Information Service");
        // Sample Characteristics.
        attributes.put(HEART_RATE_MEASUREMENT, "Heart Rate Measurement");
        attributes.put("00002a29-0000-1000-8000-00805f9b34fb", "Manufacturer Name String");
        attributes.put(SWITCH_UUID, "Switch state");
        
        attributes.put(ALERT_SERVICE, "Alert Service");
        attributes.put(ALERT_LEVEL, "Alert Level");
        attributes.put(LINK_LOSS_SERVICE, "Link Loss Service");
        attributes.put(TX_SERVICE, "TX Service");
        attributes.put(TX_POWER_LEVEL, "TX Power Level");
    }

    public static String lookup(String uuid, String defaultName) {
        String name = attributes.get(uuid);
        if(name == null) name = attributes.get(uuid.toUpperCase());
        return name == null ? defaultName : name;
    }
    
    public static boolean isHRService(String uuid){
    	return true;
//    	return uuid.equals(HEART_RATE_SERVICE);
    }
}
