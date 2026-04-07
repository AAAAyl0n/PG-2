/**
 * @file hal_rtc.cpp
 * @author Forairaaaaa
 * @brief Ref: https://github.com/m5stack/M5Unified
 * @version 0.1
 * @date 2023-11-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "../hal_rachel.h"
#include <Arduino.h>
#include "../hal_config.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <string>
#include <cstring>

void HAL_Rachel::_calibrateRTCWithCompileTime()
{
    // Initialize NVS if not already done
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
        //test6
    }

    nvs_handle_t nvs_handle;
    ret = nvs_open("timeview_rtc", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        spdlog::error("Failed to open NVS handle for RTC calibration");
        return;
    }

    // Get current stored compile time
    char stored_compile_time[32] = {0};
    size_t required_size = sizeof(stored_compile_time);
    ret = nvs_get_str(nvs_handle, "compile_time", stored_compile_time, &required_size);

    // Create current compile time string
    char current_compile_time[32];
    snprintf(current_compile_time, sizeof(current_compile_time), "%s %s", __DATE__, __TIME__);

    // Check if compile time has changed
    bool need_calibrate = false;
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // First time, need to calibrate
        need_calibrate = true;
        spdlog::info("First time RTC calibration needed");
    } else if (ret == ESP_OK) {
        // Compare with stored compile time
        if (strcmp(stored_compile_time, current_compile_time) != 0) {
            need_calibrate = true;
            spdlog::info("Compile time changed, RTC calibration needed");
            spdlog::info("Old: {}, New: {}", stored_compile_time, current_compile_time);
        } else {
            spdlog::info("Compile time unchanged, skipping RTC calibration");
        }
    } else {
        spdlog::error("Failed to read stored compile time from NVS");
    }

    if (need_calibrate) {
        // Parse compile date and time
        tm compile_tm = {0};
        
        // Parse __DATE__ (format: "MMM DD YYYY")
        char month_str[4];
        int day, year;
        sscanf(__DATE__, "%s %d %d", month_str, &day, &year);
        
        // Convert month string to number
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        for (int i = 0; i < 12; i++) {
            if (strcmp(month_str, months[i]) == 0) {
                compile_tm.tm_mon = i;
                break;
            }
        }
        
        compile_tm.tm_mday = day;
        compile_tm.tm_year = year - 1900;
        
        // Parse __TIME__ (format: "HH:MM:SS")
        int hour, min, sec;
        sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);
        compile_tm.tm_hour = hour;
        compile_tm.tm_min = min;
        compile_tm.tm_sec = sec;
        
        // Set weekday (optional, will be calculated automatically)
        compile_tm.tm_wday = -1;
        
        // Add 30 seconds to compensate for upload delay
        compile_tm.tm_sec += 31;
        
        // Handle overflow of seconds/minutes/hours
        if (compile_tm.tm_sec >= 60) {
            compile_tm.tm_sec -= 60;
            compile_tm.tm_min += 1;
            
            if (compile_tm.tm_min >= 60) {
                compile_tm.tm_min -= 60;
                compile_tm.tm_hour += 1;
                
                if (compile_tm.tm_hour >= 24) {
                    compile_tm.tm_hour -= 24;
                    compile_tm.tm_mday += 1;
                    
                    // Simple day overflow handling (not perfect for all months, but good enough)
                    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                    
                    // Check for leap year
                    bool is_leap = ((compile_tm.tm_year + 1900) % 4 == 0 && 
                                   (compile_tm.tm_year + 1900) % 100 != 0) || 
                                   (compile_tm.tm_year + 1900) % 400 == 0;
                    if (is_leap && compile_tm.tm_mon == 1) {
                        days_in_month[1] = 29;
                    }
                    
                    if (compile_tm.tm_mday > days_in_month[compile_tm.tm_mon]) {
                        compile_tm.tm_mday = 1;
                        compile_tm.tm_mon += 1;
                        
                        if (compile_tm.tm_mon >= 12) {
                            compile_tm.tm_mon = 0;
                            compile_tm.tm_year += 1;
                        }
                    }
                }
            }
        }
        
        // Set RTC time using direct member function call
        try {
            this->setSystemTime(compile_tm);
            spdlog::info("RTC calibrated with compile time: {}", current_compile_time);
            
            // Store current compile time in NVS
            ret = nvs_set_str(nvs_handle, "compile_time", current_compile_time);
            if (ret == ESP_OK) {
                ret = nvs_commit(nvs_handle);
                if (ret == ESP_OK) {
                    spdlog::info("Compile time stored in NVS successfully");
                } else {
                    spdlog::error("Failed to commit compile time to NVS");
                }
            } else {
                spdlog::error("Failed to store compile time in NVS");
            }
        } catch (...) {
            spdlog::error("Failed to set RTC time, exception occurred");
        }
    }

    nvs_close(nvs_handle);
}

void HAL_Rachel::_rtc_init()
{
    spdlog::info("rtc init");
    HAL_LOG_INFO("rtc init");

    _rtc = new m5::RTC8563_Class(0x51, 400000, _i2c_bus);
    if (!_rtc->begin())
    {
        spdlog::error("rtc init failed!");
        HAL_LOG_ERROR("rtc init failed!");
    }

    _adjust_sys_time();
    
    // Calibrate RTC with compile time if needed
    _calibrateRTCWithCompileTime();

    // // Test
    // tm set_time;
    // set_time.tm_year = 2023;
    // set_time.tm_mon = 10;
    // set_time.tm_mday = 8;
    // set_time.tm_wday = 3;
    // set_time.tm_hour = 16;
    // set_time.tm_min = 15;
    // set_time.tm_sec = 0;
    // this->setDateTime(set_time);

    // m5::rtc_time_t rtc_time;
    // m5::rtc_date_t rtc_date;
    // while (1)
    // {
    //     _rtc->getTime(&rtc_time);
    //     _rtc->getDate(&rtc_date);
    //     printf("%02d:%02d:%02d %04d,%02d,%02d,%02d\n",
    //         rtc_time.hours,
    //         rtc_time.minutes,
    //         rtc_time.seconds,
    //         rtc_date.year,
    //         rtc_date.month,
    //         rtc_date.date,
    //         rtc_date.weekDay
    //     );
    //     this->delay(1000);
    // }
}

void HAL_Rachel::setSystemTime(tm dateTime)
{
    _rtc->setDateTime(dateTime);
    _adjust_sys_time();
}

void HAL_Rachel::_adjust_sys_time()
{
    auto time = _rtc->getDateTime().get_tm();
    time_t tv_sec = mktime(&time);
    struct timeval now = {.tv_sec = tv_sec};
    settimeofday(&now, NULL);
    spdlog::info("adjusted system time to rtc");
}
