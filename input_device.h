/**
 * @file     input_device.h
 * @brief    输入设备组件（按键 + 触摸）— C++ class
 *
 * 功能：LVGL 按键（keypad）与触摸（pointer）输入设备注册、
 *       双击检测、复位组合键长按检测、触摸滑动翻页。
 * 修改：2026-11-21 由 C API（keypad_*/touchpad_*/input_device_* 函数）重构为 InputDevice 静态类；
 *       硬件读取由桩函数改为可注入回调（setKeyReader/setTouchReader），
 *       未注入时保持原行为（无按键/无触摸），便于接入真实硬件；
 *       修复触摸读取回调设置 enc_diff（属编码器字段）对 POINTER 类型无效的问题
 */
#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include "lvgl.h"
#include <stdint.h>

/* 输入设备使能开关（可在 build_flags 覆盖） */
#ifndef ENABLE_KEYPAD_INPUT
#define ENABLE_KEYPAD_INPUT 1
#endif
#ifndef ENABLE_TOUCHPAD_INPUT
#define ENABLE_TOUCHPAD_INPUT 1
#endif

/**
 * 输入事件类型
 */
enum class InputEventType : uint8_t {
    SINGLE_CLICK, /*!< 单击 */
    DOUBLE_CLICK, /*!< 双击 */
    KEY_PRESS,    /*!< 按键按下（复位组合键长按达标时以此上报） */
    KEY_RELEASE,  /*!< 按键释放 */
};

/**
 * 输入事件
 */
struct InputEvent {
    InputEventType type;
    lv_key_t key;
};

/** 输入事件回调 */
using InputEventCb = void (*)(const InputEvent &event);

/**
 * 硬件按键读取回调（新功能：注入真实硬件读取）
 * @return 当前按下的键（LV_KEY_*）；无按键返回 0
 */
using KeyReaderCb = lv_key_t (*)(void);

/**
 * 触摸读取回调（新功能：注入真实硬件读取）
 * @param x 输出触摸点 X
 * @param y 输出触摸点 Y
 * @return 是否按下
 */
using TouchReaderCb = bool (*)(int16_t &x, int16_t &y);

/** 复位组合键检测回调；返回 true 表示组合键处于按下状态 */
using ResetCombineCb = bool (*)(void);

/**
 * 输入设备静态类
 *
 * 用法：
 *   InputDevice::setKeyReader(myKeyRead);      // 可选：接入真实按键硬件
 *   InputDevice::setTouchReader(myTouchRead);  // 可选：接入真实触摸硬件
 *   InputDevice::init(onInputEvent);           // 按宏开关创建 LVGL 输入设备
 */
class InputDevice {
public:
    static constexpr uint32_t DOUBLE_CLICK_TIMEOUT_MS = 300; /*!< 双击判定间隔 */
    static constexpr uint32_t RESET_COMBINE_HOLD_MS = 3000;  /*!< 复位组合键长按时长 */
    static constexpr int SWIPE_THRESHOLD_PX = 50;            /*!< 滑动翻页阈值 */

    /** 初始化按键输入（创建 keypad indev 与输入组） */
    static void initKeypad(InputEventCb cb = nullptr);

    /** 反初始化按键输入 */
    static void deinitKeypad();

    /** 初始化触摸输入（创建 pointer indev） */
    static void initTouchpad(InputEventCb cb = nullptr);

    /** 反初始化触摸输入 */
    static void deinitTouchpad();

    /** 按 ENABLE_KEYPAD_INPUT / ENABLE_TOUCHPAD_INPUT 开关初始化全部输入设备 */
    static void init(InputEventCb cb = nullptr);

    /** 反初始化全部输入设备 */
    static void deinit();

    /** 注入硬件按键读取（不注入则恒为无按键，与原行为一致） */
    static void setKeyReader(KeyReaderCb cb);

    /** 注入触摸读取（不注入则恒为无触摸，与原行为一致） */
    static void setTouchReader(TouchReaderCb cb);

    /** 注入复位组合键检测（不注入则恒为未按，与原行为一致） */
    static void setResetCombineChecker(ResetCombineCb cb);

    /** 获取按键输入组（供应用把自定义对象加入按键导航） */
    static lv_group_t *group();

private:
    static void keypadRead(lv_indev_t *indev, lv_indev_data_t *data);
    static void touchpadRead(lv_indev_t *indev, lv_indev_data_t *data);
    static bool isDoubleClickDetected();
    static uint32_t nowMs();

    static InputEventCb s_eventCb;
    static KeyReaderCb s_keyReader;
    static TouchReaderCb s_touchReader;
    static ResetCombineCb s_resetChecker;

    static lv_indev_t *s_keypadIndev;
    static lv_indev_t *s_touchpadIndev;
    static lv_group_t *s_group;

    /* 双击检测状态 */
    static uint32_t s_lastClickTime;
    static bool s_firstClickDetected;

    /* 复位组合键长按状态 */
    static uint32_t s_resetHoldStart;
    static bool s_resetHolding;

    /* 触摸滑动状态 */
    static int s_touchStartX;
    static int s_touchStartY;
    static bool s_touchScrolling;
};

#endif /* INPUT_DEVICE_H */
