/**
 * @file     input_device.cpp
 * @brief    输入设备实现
 *
 * 修改：2026-11-21 重构为 InputDevice 静态类；硬件读取改可注入回调
 */
#include "input_device.h"
#include "ui_manager.h"

#include <cstdlib>

InputEventCb InputDevice::s_eventCb = nullptr;
KeyReaderCb InputDevice::s_keyReader = nullptr;
TouchReaderCb InputDevice::s_touchReader = nullptr;
ResetCombineCb InputDevice::s_resetChecker = nullptr;

lv_indev_t *InputDevice::s_keypadIndev = nullptr;
lv_indev_t *InputDevice::s_touchpadIndev = nullptr;
lv_group_t *InputDevice::s_group = nullptr;

uint32_t InputDevice::s_lastClickTime = 0;
bool InputDevice::s_firstClickDetected = false;

uint32_t InputDevice::s_resetHoldStart = 0;
bool InputDevice::s_resetHolding = false;

int InputDevice::s_touchStartX = 0;
int InputDevice::s_touchStartY = 0;
bool InputDevice::s_touchScrolling = false;

uint32_t InputDevice::nowMs()
{
    return lv_tick_get();
}

void InputDevice::setKeyReader(KeyReaderCb cb)
{
    s_keyReader = cb;
}

void InputDevice::setTouchReader(TouchReaderCb cb)
{
    s_touchReader = cb;
}

void InputDevice::setResetCombineChecker(ResetCombineCb cb)
{
    s_resetChecker = cb;
}

lv_group_t *InputDevice::group()
{
    return s_group;
}

/**
 * @brief 双击检测：两次 ENTER 间隔不超过超时时间判为双击
 */
bool InputDevice::isDoubleClickDetected()
{
    uint32_t now = nowMs();
    if (s_firstClickDetected) {
        s_firstClickDetected = false;
        if (now - s_lastClickTime <= DOUBLE_CLICK_TIMEOUT_MS) {
            return true;
        }
    }
    return false;
}

void InputDevice::keypadRead(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    uint32_t key = (s_keyReader != nullptr) ? (uint32_t)s_keyReader() : 0;

    if (isDoubleClickDetected()) {
        data->key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key != 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = key;

        if (key == LV_KEY_ENTER && !s_firstClickDetected) {
            s_firstClickDetected = true;
            s_lastClickTime = nowMs();
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /* 复位组合键长按检测 */
    bool combine = (s_resetChecker != nullptr) && s_resetChecker();
    if (combine) {
        if (!s_resetHolding) {
            s_resetHolding = true;
            s_resetHoldStart = nowMs();
        } else if (nowMs() - s_resetHoldStart >= RESET_COMBINE_HOLD_MS) {
            if (s_eventCb != nullptr) {
                InputEvent event = { InputEventType::KEY_PRESS, LV_KEY_ENTER };
                s_eventCb(event);
            }
            s_resetHolding = false;
        }
    } else {
        s_resetHolding = false;
    }
}

void InputDevice::touchpadRead(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    int16_t x = 0;
    int16_t y = 0;
    bool pressed = (s_touchReader != nullptr) && s_touchReader(x, y);

    if (pressed) {
        data->point.x = x;
        data->point.y = y;
    }
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    if (pressed) {
        if (!s_touchScrolling) {
            s_touchStartX = x;
            s_touchStartY = y;
            s_touchScrolling = true;
        }
    } else if (s_touchScrolling) {
        /* 抬手时按最后坐标与起点计算滑动量，水平滑动超阈值触发翻页 */
        int delta_x = data->point.x - s_touchStartX;
        int delta_y = data->point.y - s_touchStartY;

        if (abs(delta_x) > abs(delta_y) && abs(delta_x) > SWIPE_THRESHOLD_PX) {
            if (delta_x > 0) {
                UiManager::instance().prevPage();
            } else {
                UiManager::instance().nextPage();
            }
        }

        s_touchScrolling = false;
    }
}

void InputDevice::initKeypad(InputEventCb cb)
{
    if (cb != nullptr) {
        s_eventCb = cb;
    }
    if (s_keypadIndev != nullptr) {
        return; /* 已初始化 */
    }

    s_group = lv_group_create();

    s_keypadIndev = lv_indev_create();
    lv_indev_set_type(s_keypadIndev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keypadIndev, keypadRead);
    lv_indev_set_group(s_keypadIndev, s_group);
}

void InputDevice::deinitKeypad()
{
    if (s_keypadIndev != nullptr) {
        lv_indev_delete(s_keypadIndev);
        s_keypadIndev = nullptr;
    }
    if (s_group != nullptr) {
        lv_group_delete(s_group);
        s_group = nullptr;
    }
}

void InputDevice::initTouchpad(InputEventCb cb)
{
    if (cb != nullptr) {
        s_eventCb = cb;
    }
    if (s_touchpadIndev != nullptr) {
        return; /* 已初始化 */
    }

    s_touchpadIndev = lv_indev_create();
    lv_indev_set_type(s_touchpadIndev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touchpadIndev, touchpadRead);
}

void InputDevice::deinitTouchpad()
{
    if (s_touchpadIndev != nullptr) {
        lv_indev_delete(s_touchpadIndev);
        s_touchpadIndev = nullptr;
    }
}

void InputDevice::init(InputEventCb cb)
{
    if (cb != nullptr) {
        s_eventCb = cb;
    }

#if ENABLE_KEYPAD_INPUT
    initKeypad();
#endif

#if ENABLE_TOUCHPAD_INPUT
    initTouchpad();
#endif
}

void InputDevice::deinit()
{
#if ENABLE_KEYPAD_INPUT
    deinitKeypad();
#endif

#if ENABLE_TOUCHPAD_INPUT
    deinitTouchpad();
#endif

    s_eventCb = nullptr;
}
