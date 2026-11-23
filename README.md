# UI Utils 组件 v1.0（C++ class 版）

## 一、组件概述

UI Utils 是基于 LVGL 的 UI 组件库，提供桌面系统架构、多输入设备支持、设置界面、恢复出厂设置触发、版本检查与通用 UI 工具。

### 特性

- **桌面系统**：`UiManager` 单例管理应用注册/启动、桌面图标网格分页、布局切换、页码指示与 UI 事件广播
- **输入设备**：`InputDevice` 提供按键（keypad）与触摸（pointer）LVGL 输入设备，支持双击检测、复位组合键长按检测、触摸滑动翻页；硬件读取可注入
- **设置界面**：`SettingsUi` 单例提供 8 项设置菜单（系统信息/网络/蓝牙/声音/显示/按键/桌面布局/恢复默认）与设置数据管理
- **恢复出厂**：`UiReset` 组合键长按触发恢复默认设置并广播复位事件
- **版本检查**：`VersionCheck` 版本号比较与更新信息管理，版本源可注入
- **通用工具**：`UiUtils` WiFi 信号图标绘制（canvas）、中文标签创建、对象位置/尺寸设置

### 模块组成

```
┌─────────────────────────────────────────────┐
│           UiManager (桌面管理器)             │
│  - 应用注册/注销/启动                        │
│  - 图标网格分页 / 布局切换 / 事件广播        │
├─────────────────────────────────────────────┤
│           InputDevice (输入设备)             │
│  - 按键驱动 (Keypad) + 输入组                │
│  - 触摸驱动 (Touchpad) + 滑动翻页            │
├─────────────────────────────────────────────┤
│            SettingsUi (设置界面)             │
│  - 系统信息/网络/蓝牙/声音/显示/按键          │
│  - 桌面布局/恢复默认                         │
├─────────────────────────────────────────────┤
│   UiReset (恢复出厂)   VersionCheck (版本)   │
├─────────────────────────────────────────────┤
│              UiUtils (通用工具)              │
│  - WiFi 图标 / 标签 / 位置尺寸               │
└─────────────────────────────────────────────┘
```

---

## 二、目录结构

```
ui_utils/
├── ui_utils.h / ui_utils.cpp          # UiUtils 通用 UI 工具静态类
├── ui_manager.h / ui_manager.cpp      # UiManager 桌面管理器单例
├── input_device.h / input_device.cpp  # InputDevice 输入设备静态类
├── settings_ui.h / settings_ui.cpp    # SettingsUi 设置界面单例
├── ui_reset.h / ui_reset.cpp          # UiReset 恢复出厂触发静态类
├── version_check.h / version_check.cpp# VersionCheck 版本检查静态类
├── library.json                       # PlatformIO 库描述
└── README.md                          # 开发文档（本文件）
```

---

## 三、快速上手

```cpp
#include "ui_manager.h"
#include "input_device.h"
#include "settings_ui.h"
#include "ui_reset.h"

/* 1. 可选：注入真实硬件读取（不注入则无按键/无触摸，与原行为一致） */
InputDevice::setKeyReader([]() -> lv_key_t {
    /* 读按键矩阵/ADC，返回 LV_KEY_*；无按键返回 0 */
    return 0;
});
InputDevice::setTouchReader([](int16_t &x, int16_t &y) -> bool {
    /* 读触摸控制器；按下时填 x/y 并返回 true */
    return false;
});

/* 2. 初始化桌面与输入设备 */
UiManager::instance().init();
InputDevice::init();

/* 3. 注册应用 */
UiManager::instance().registerApp({
    .id = 1,
    .name = "Settings",
    .iconType = IconType::SYMBOL,
    .icon = LV_SYMBOL_SETTINGS,
    .launchCb = [] { SettingsUi::instance().show(); },
    .visible = true,
});

/* 4. 恢复出厂：注入组合键检测并周期调用 check */
UiReset::setCombinationChecker([] { return /* 组合键是否按下 */ false; });
UiReset::init();
// 主循环中：UiReset::check();
```

---

## 四、API 概览

### 4.1 UiUtils（通用工具，静态类）

| 方法 | 说明 |
|------|------|
| `createWifiIcon(parent, x, y, size, level)` | 创建 WiFi 信号图标（canvas，缓冲自动随对象释放） |
| `updateWifiIcon(canvas, level)` | 更新信号等级（重绘） |
| `drawWifiIcon(canvas, x, y, size, level)` | 在已有 canvas 上绘制 |
| `createChineseLabel(parent, text, font, align, x, y)` | 创建带字体与对齐的标签 |
| `setLabelText / setLabelFont / setLabelAlign` | 标签属性设置（空参数防护） |
| `setObjPos / setObjSize` | 对象位置/尺寸（空参数防护） |

信号等级枚举：`WifiSignalLevel::NONE / WEAK / MEDIUM / STRONG`。

### 4.2 UiManager（桌面管理器，单例）

| 方法 | 说明 |
|------|------|
| `init()` | 初始化桌面（重复调用安全，仅刷新图标） |
| `registerApp(app) / unregisterApp(id)` | 注册/注销应用，自动刷新桌面 |
| `launchApp(id)` | 启动应用（清屏 → 建页面容器 → 调启动回调） |
| `returnToDesktop()` | 返回桌面（输入组不重复创建） |
| `addEventListener(cb, userData)` | 注册 UI 事件监听（最多 8 个） |
| `notifyEvent(event)` | 广播事件（RESET_TRIGGERED / PAGE_CHANGED） |
| `setLayout(layout) / layout()` | 桌面布局（2x2 / 2x3 / 3x3 / 4x2 / 4x3，默认 4x3） |
| `currentPage() / totalPages() / setPage / nextPage / prevPage` | 分页控制（翻页广播 PAGE_CHANGED） |
| `setAppVisible(id, visible)` | 应用显隐并刷新 |
| `currentLaunchingAppId()` | 当前运行的应用 ID |

应用描述 `AppDesc`：`id / name / iconType(NONE|IMAGE|SYMBOL) / icon / launchCb / visible`。

### 4.3 InputDevice（输入设备，静态类）

| 方法 | 说明 |
|------|------|
| `init(cb) / deinit()` | 按宏开关初始化/反初始化全部输入设备 |
| `initKeypad(cb) / deinitKeypad()` | 按键输入（创建 keypad indev 与输入组） |
| `initTouchpad(cb) / deinitTouchpad()` | 触摸输入（创建 pointer indev） |
| `setKeyReader(cb)` | 注入硬件按键读取（新功能） |
| `setTouchReader(cb)` | 注入触摸读取（新功能） |
| `setResetCombineChecker(cb)` | 注入复位组合键检测（新功能） |
| `group()` | 获取按键输入组（供应用加入自定义对象） |

编译开关（可在 `build_flags` 覆盖，默认均为 1）：

- `ENABLE_KEYPAD_INPUT`
- `ENABLE_TOUCHPAD_INPUT`

内置行为：双击判定间隔 300ms；复位组合键长按 3000ms 达标以 `KEY_PRESS + LV_KEY_ENTER` 事件上报；抬手时水平滑动超 50px 触发桌面翻页。

### 4.4 SettingsUi（设置界面，单例）

| 方法 | 说明 |
|------|------|
| `init() / deinit()` | 初始化/销毁设置页 |
| `show() / hide()` | 显示/隐藏（已显示时忽略，防重复创建） |
| `isShown()` | 是否正在显示 |
| `setVersion(project, component)` | 设置版本信息（系统信息页显示） |
| `updateWifiStatus(ssid, ip, level, connected)` | 更新 WiFi 状态 |
| `updateBluetoothStatus(enabled, name)` | 更新蓝牙状态 |
| `volume() / setVolume(v)` | 音量（自动钳位 0-100） |
| `brightness() / setBrightness(v)` | 亮度（自动钳位 0-100） |
| `keySensitivity() / setKeySensitivity(v)` | 按键灵敏度（自动钳位 0-100） |
| `resetToDefaults()` | 恢复默认设置（显示中同步刷新） |
| `data()` | 只读访问完整设置数据 |

菜单项：系统信息、网络、蓝牙、声音、显示、按键、桌面布局（切换 `UiManager` 布局）、恢复默认。

### 4.5 UiReset（恢复出厂，静态类）

| 方法 | 说明 |
|------|------|
| `init(cb) / deinit()` | 初始化/反初始化（cb 为触发回调） |
| `check()` | 周期检查（主循环调用） |
| `isTriggered() / clearTriggered()` | 触发标志读写 |
| `setCombinationChecker(cb)` | 注入组合键检测（新功能） |
| `setHoldDuration(ms) / holdDuration()` | 长按时长配置（默认 3000ms） |

触发流程：组合键持续按下达到时长 → `SettingsUi::resetToDefaults()` → 触发回调 → 广播 `UiEvent::RESET_TRIGGERED`。触发后锁存，松开前不重复触发。

### 4.6 VersionCheck（版本检查，静态类）

| 方法 | 说明 |
|------|------|
| `init() / deinit()` | 预留 |
| `start() / stop()` | 开始/中止检查（检查完成自动计算 updateAvailable） |
| `info()` | 只读版本信息 |
| `hasUpdate()` | 是否有可用更新 |
| `compareVersions(v1, v2)` | 语义化版本号比较（改 public，支持 v/V 前缀） |
| `setCurrentVersion(version)` | 设置当前固件版本（新功能） |
| `setUpdateSource(cb)` | 注入真实版本源检查（新功能；不注入用内置模拟数据 v1.2.0） |

`VersionInfo` 字段：`currentVersion / latestVersion / updateLog / packageSize / updateAvailable / checkInProgress`。

---

## 五、技术说明（本次重构要点）

### BUG 修复

| 模块 | 问题 | 修复 |
|------|------|------|
| UiUtils | canvas 缓冲尺寸未按 I1 行距对齐 | 改用 `lv_draw_buf_width_to_stride` 计算 |
| UiUtils | canvas 缓冲存 user_data 后删除对象永不释放（内存泄漏） | 挂 `LV_EVENT_DELETE` 事件自动释放 |
| UiManager | `returnToDesktop` 重复调用 init 导致 lv_group 与页码标签重复创建泄漏 | 输入组仅创建一次；页码指示挂删除事件 |
| UiManager | 图标按钮 user_data 用 malloc 存 app_id（内存管理问题） | 改 intptr 直存指针值 |
| UiManager | launchApp 清屏后桌面/页码指针悬空 | 挂删除事件回调置空 |
| UiManager | `notifyEvent` 为空实现 | 支持注册监听器分发 |
| InputDevice | 触摸读取回调误设置 enc_diff（编码器字段，对 POINTER 无效） | 移除，正确设置 point |
| SettingsUi | 菜单/布局按钮 user_data 用 malloc 存整数 | 改 intptr 直存 |
| SettingsUi | 菜单点击回调先删子页再读被删按钮 user_data（use-after-free） | 先取值再删页 |
| SettingsUi | 重复调用 show 重复创建页面 | 已显示时忽略 |
| SettingsUi | 返回键在菜单页删掉菜单留下空白页 | 菜单/子页分开管理 |
| UiReset | `get_current_time_ms` 恒返回 0，长按计时失效 | 改用 `esp_timer_get_time()/1000` |
| UiReset | 组合键检测恒返回 false，永远无法触发 | 改为可注入回调 |
| UiReset | 触发后继续按住会周期性重复触发 | 新增触发锁存，松开后解除 |

### 新增功能

- **硬件读取注入**：`InputDevice::setKeyReader / setTouchReader / setResetCombineChecker`，未注入时保持原行为（无按键/无触摸），便于接入真实硬件
- **复位组合键注入**：`UiReset::setCombinationChecker`，长按时长可配置（`setHoldDuration`）
- **版本源注入**：`VersionCheck::setUpdateSource` 可接 HTTP 等真实检查逻辑；`setCurrentVersion` 设置当前版本
- **事件监听**：`UiManager::addEventListener` 支持最多 8 个监听器接收复位/翻页事件

### 其他变更

- 全部 C API（`ui_*` / `ui_manager_*` / `settings_ui_*` / `ui_reset_*` / `version_check_*` 函数）重构为 C++ class，不保留兼容层
- `lv_img_*` 弃用 API 换为 `lv_image_*`
- 字符串拷贝统一加终止符防护
- canvas 缓冲改用 `lv_malloc` 分配，与 LVGL 内存管理一致

---

## 六、版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0.0 | 2026-11-21 | 重构为 C++ class（UiUtils/UiManager/InputDevice/SettingsUi/UiReset/VersionCheck）；修复内存泄漏、use-after-free、悬空指针、计时恒 0、组合键恒 false 等问题；新增硬件读取/组合键/版本源注入与事件监听 |
| - | - | ESP-IDF C 版本（ui_* 函数 API） |

## 许可证

MIT License
