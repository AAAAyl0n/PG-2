/**
 * @file state_machine.cpp
 * @brief 简单而可扩展的状态机实现
 * @author Assistant
 * @date 2025-01-24
 */

#include "app_bangboo.h"
#include "../../hal/hal.h"
#include "spdlog/spdlog.h"
#include <cstdlib>
#include "esp_system.h"

using namespace MOONCAKE::APPS;

// 状态机更新函数 - 在主循环中调用
void AppBangboo::updateStateMachine() {
    uint32_t currentTime = HAL::Millis();
    uint32_t stateElapsed = currentTime - _data.stateStartTime;
    
    // 更新当前表情
    _data.currentExpression = getExpressionForState(_data.currentState);
    
    // 全局高优先级：仅在可中断的空闲类状态下响应摇晃
    if (_data.shakeState == SHAKE_TRIGGERED) {
        switch (_data.currentState) {
            case STATE_IDLE:
            case STATE_PREIDLE:
            case STATE_BLINKING0:
            case STATE_BLINKTEMP:
            case STATE_BLINKING1:
            case STATE_SLEEPING:
            case STATE_POSTSLEEP:
            case STATE_SLEEPEND:
                changeState(STATE_SHAKED);
                return;
            default:
                break;
        }
    }

    // 状态转换逻辑
    switch (_data.currentState) {

        case STATE_PREIDLE:
            if (stateElapsed > 100) {
                // 切空闲前弹个短提示
                changeState(STATE_IDLE);
                //showStatusMessage("^^_", 2000);
            }
            break;

        case STATE_IDLE:
            // 空闲状态 - 随机选择下一个状态
            if (stateElapsed > (5500 + rand() % 3000)) {
                    int nextState = esp_random() % 7;
                    switch (nextState) {
                        case 0: changeState(STATE_BLINKING0); break;
                        case 1: changeState(STATE_BLINKING0); break;
                        case 2: changeState(STATE_BLINKING0); break;
                        case 3: changeState(STATE_BLINKING0); break;
                        case 4: changeState(STATE_BLINKING0); break;
                        case 5: changeState(STATE_PREHAPPY); break;
                        case 6: changeState(STATE_SLEEPING); break;
                    }
            }
            break;
        
        //眨眼
        case STATE_BLINKING0:
            // 眨眼状态 - 短暂眨眼后回到空闲
            if (stateElapsed > 100) {
                // INSERT_YOUR_CODE
                // 0.6的概率去IDLE，0.4的概率去BLINKTEMP
                if ((esp_random() % 10) < 4) {
                    changeState(STATE_BLINKTEMP);
                } else {
                    changeState(STATE_IDLE);
                }
            }
            break;
        case STATE_BLINKTEMP:
            if (stateElapsed > 100) {
                changeState(STATE_BLINKING1);
            }
            break;
            
        case STATE_BLINKING1:
            // 眨眼状态 - 短暂眨眼后回到空闲
            if (stateElapsed > 100) {
                changeState(STATE_IDLE);
            }
            break;
            
        //笑
        case STATE_PREHAPPY:
            if (stateElapsed > 100) {
                changeState(STATE_HAPPY);
            }
            break;
        case STATE_HAPPY:
            // 开心状态 - 保持1-2秒后回到空闲
            if (stateElapsed > (2000 + rand() % 1000)) {
                changeState(STATE_PREIDLE);
            }
            break;
        
        //我操 用户彻底怒了
        case STATE_PREANGER:
            if (stateElapsed > 100) {
                changeState(STATE_ANGER);
            }
            break;
        case STATE_ANGER:
            // 愤怒状态 - 保持1-2秒后回到空闲
            if (stateElapsed > (2000 + rand() % 1000)) {
                changeState(STATE_PREIDLE);
            }
            break;

        //摇
        case STATE_SHAKED:
            if(stateElapsed > 10) {
                int nextState = esp_random() % 6;
                switch (nextState) {
                    case 0: changeState(STATE_PREWINCE1); break;
                    case 1: changeState(STATE_PREWINCE2); HAL::PlayWavFile("/bangboo_audio/frustrated.wav");break;
                    case 2: {
                        showStatusMessage("!", 2000);
                        changeState(STATE_PREANGER); 
                        HAL::PlayWavFile("/bangboo_audio/angry.wav");
                        break;
                    }
                    case 3: changeState(STATE_CURIOUS); break;
                    case 4: changeState(STATE_DULL); break;
                    case 5: changeState(STATE_CURIOUS); break;
                }
                //changeState(STATE_IDLE);
            }
            break;


        //给兄弟晃晕了
        case STATE_PREWINCE1:
            if (stateElapsed > 10) {
                changeState(STATE_WINCE);
                {
                    //int idx = esp_random() % 2;
                    //const char* audio_files[] = {
                    //    "/bangboo_audio/1s_4.wav",
                    //    "/bangboo_audio/1s_5.wav"
                    //};
                    HAL::PlayWavFile("/bangboo_audio/1s_5.wav");
                }
            }
            break;
        case STATE_PREWINCE2:
            if (stateElapsed > 1800) {
                changeState(STATE_PRESAD);
            }
            break;
        case STATE_WINCE:
            if (stateElapsed > 1800) {
                changeState(STATE_POSTWINCE);
            }
            break;
        case STATE_POSTWINCE:
            if (stateElapsed > 100) {
                changeState(STATE_IDLE);
            }
            break; 

        //悲伤
        case STATE_PRESAD:
            if (stateElapsed > 200) {
                changeState(STATE_SAD);
                showStatusMessage("QAQ", 1600);
            }
            break;
        case STATE_SAD:
            if (stateElapsed > 1600) {
                changeState(STATE_POSTSAD);
            }
            break;
        case STATE_POSTSAD:
            if (stateElapsed > 150) {
                changeState(STATE_IDLE);
            }
            break;

        //呆滞
        case STATE_DULL:
            if (stateElapsed > 100) {
                showStatusMessage("O.o", 3000);
                changeState(STATE_IDLE);
                HAL::PlayWavFile("/bangboo_audio/dull.wav");
            }
            break;

        //好奇
        case STATE_CURIOUS:
            if (stateElapsed > 100) {
                showStatusMessage("?", 2000);
                changeState(STATE_BLINKING0);
                int idx = esp_random() % 3;
                const char* audio_files[] = {
                    "/bangboo_audio/curious0.wav",
                    "/bangboo_audio/curious1.wav",
                    "/bangboo_audio/curious2.wav"
                };
                HAL::PlayWavFile(audio_files[idx]);
            }
            break;
        
        //年轻就是好 两眼一闭就是睡
        case STATE_SLEEPING: {
            // 进入睡眠时重置索引
            if (_data.sleepZLastIndex < 0) {
                _data.sleepZLastIndex = 2; 
            }

            // 循环 "z" -> "zz" -> "zzz"
            const uint32_t step = 700;
            uint32_t phase = (stateElapsed / step) % 60;
            if ((int)phase != _data.sleepZLastIndex) {
                _data.sleepZLastIndex = (int)phase;
                switch (phase) {
                    case 0: showStatusMessage(".  ", step + 1200); break;
                    case 1: showStatusMessage(".z ", step + 1200); break;
                    case 2: showStatusMessage(".zZ", step + 1200); break;
                    case 3: showStatusMessage(" zZ", step + 1200); break;
                    case 4: showStatusMessage("  Z", step + 1200); break;
                    case 5: showStatusMessage("   ", step + 1200); break;
                    case 6: showStatusMessage(".  ", step + 1200); break;
                    case 7: showStatusMessage(".z ", step + 1200); break;
                    case 8: showStatusMessage(".zZ", step + 1200); break;
                    case 9: showStatusMessage(" zZ", step + 1200); break;
                    case 10: showStatusMessage("  Z", step + 1200); break;      
                    case 11: showStatusMessage("   ", step - 600); break;                
                }
            }
            if (stateElapsed > 10000) {
                //showStatusMessage("-o-", 1000);
                changeState(STATE_POSTSLEEP);
            }
            break;
        }
        case STATE_POSTSLEEP:
            if (stateElapsed > 1000) {
                //showStatusMessage("+++", 1000);
                changeState(STATE_SLEEPEND);
            }
            break;
        case STATE_SLEEPEND:
            if (stateElapsed > 100) {
                showStatusMessage("owo", 1000);
                changeState(STATE_IDLE);
            }
            break;
    }
}

// 改变状态
void AppBangboo::changeState(BangbooState_t newState) {
    if (_data.currentState != newState) {
        // 如果从睡眠状态切换到其他状态，清理睡眠相关状态
        //if (_data.currentState == STATE_SLEEPING) {
        //    _data.sleepZLastIndex = -1;
        //    // 清理状态栏消息并隐藏状态栏
        //    _data.statusBarMessage.clear();
        //    _data.statusBarMessageExpire = 0;
        //}
        
        _data.currentState = newState;
        _data.stateStartTime = HAL::Millis();
        
        // 调试信息
        const char* stateNames[] = {"SLEEPING", "IDLE", "HAPPY", "BLINKING", "ANGER", "WINCE", "SAD", "DULL", "CURIOUS", "SHAKED"};
        spdlog::debug("状态改变: {}", stateNames[newState]);
    }
}

// 根据状态获取对应的表情
AppBangboo::ExpressionType_t AppBangboo::getExpressionForState(BangbooState_t state) {
    switch (state) {
        case STATE_SLEEPING:  return EXPR_SLEEP;
        case STATE_POSTSLEEP:  return EXPR_SLEEP;
        case STATE_SLEEPEND:  return EXPR_EYES;

        case STATE_PREIDLE:   return EXPR_BLINK;
        case STATE_IDLE:      return EXPR_EYES;

        case STATE_PREHAPPY:  return EXPR_BLINK;
        case STATE_HAPPY:     return EXPR_SMILE;

        case STATE_BLINKING0:  return EXPR_BLINK;
        case STATE_BLINKTEMP:  return EXPR_EYES;
        case STATE_BLINKING1:  return EXPR_BLINK;

        case STATE_PREANGER:  return EXPR_BLINK;
        case STATE_ANGER:      return EXPR_ANGER;

        case STATE_PREWINCE1: return EXPR_WINCE;
        case STATE_PREWINCE2:  return EXPR_WINCE;
        case STATE_WINCE:     return EXPR_WINCE;
        case STATE_POSTWINCE: return EXPR_BLINK;

        case STATE_PRESAD:  return EXPR_BLINK;
        case STATE_SAD:     return EXPR_SAD;
        case STATE_POSTSAD: return EXPR_BLINK;

        case STATE_DULL:      return EXPR_EYES;
        case STATE_CURIOUS:   return EXPR_EYES;

        case STATE_SHAKED:   return EXPR_EYES;

        default:              return EXPR_EYES;
    }
}
