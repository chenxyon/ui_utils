/**
 * @file     settings_ui.cpp
 * @brief    设置界面实现
 *
 * 修改：2026-11-21 重构为 SettingsUi 单例；修复 user_data 内存管理、
 *       use-after-free、重复 show、返回键空白页等问题（详见头文件）
 */
#include "settings_ui.h"
#include "ui_manager.h"

#include <cstdio>
#include <cstring>

SettingsUi &SettingsUi::instance()
{
    static SettingsUi inst;
    return inst;
}

SettingsUi::SettingsUi()
{
    resetToDefaults();
}

void SettingsUi::safeCopy(char *dst, size_t dstSize, const char *src)
{
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    if (src == nullptr) {
        src = "";
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0'; /* 确保终止符 */
}

int SettingsUi::clampPercent(int value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}

void SettingsUi::init()
{
}

void SettingsUi::deinit()
{
    if (m_page != nullptr) {
        lv_obj_del(m_page); /* pageDeleteCb 会置空各指针 */
    }
}

void SettingsUi::hide()
{
    deinit();
}

void SettingsUi::show()
{
    if (m_page != nullptr) {
        return; /* 已显示，避免重复创建（修复原实现） */
    }
    buildPage();
}

void SettingsUi::pageDeleteCb(lv_event_t *e)
{
    SettingsUi *self = (SettingsUi *)lv_event_get_user_data(e);
    self->m_page = nullptr;
    self->m_menu = nullptr;
    self->m_subpage = nullptr;
}

void SettingsUi::buildPage()
{
    m_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_event_cb(m_page, pageDeleteCb, LV_EVENT_DELETE, this);

    createBackBtn(m_page);

    lv_obj_t *title = lv_label_create(m_page);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    showMenu();
}

void SettingsUi::createBackBtn(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 60, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Back");
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, backBtnCb, LV_EVENT_CLICKED, this);
}

void SettingsUi::backBtnCb(lv_event_t *e)
{
    SettingsUi *self = (SettingsUi *)lv_event_get_user_data(e);

    if (self->m_subpage != nullptr) {
        /* 子页返回菜单（修复原实现菜单被删后空白页的问题） */
        lv_obj_del(self->m_subpage);
        self->m_subpage = nullptr;
        self->showMenu();
    } else {
        UiManager::instance().returnToDesktop();
    }
}

/**
 * @brief 创建子页通用容器（居中、纵向 flex）
 */
lv_obj_t *SettingsUi::createSubpageContainer(lv_obj_t *parent, int padRow)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES - 40, LV_VER_RES - 100);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, padRow, 0);
    return cont;
}

void SettingsUi::createSystemInfoPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 20);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "System Info");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    char buf[64];
    snprintf(buf, sizeof(buf), "Project Version: %s", m_settings.projectVersion);
    lv_obj_t *project_ver = lv_label_create(cont);
    lv_label_set_text(project_ver, buf);

    snprintf(buf, sizeof(buf), "Component Version: %s", m_settings.componentVersion);
    lv_obj_t *component_ver = lv_label_create(cont);
    lv_label_set_text(component_ver, buf);

    m_subpage = cont;
}

void SettingsUi::createNetworkPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 15);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Network");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    char buf[64];
    snprintf(buf, sizeof(buf), "SSID: %s", m_settings.ssid);
    lv_obj_t *ssid_label = lv_label_create(cont);
    lv_label_set_text(ssid_label, buf);

    snprintf(buf, sizeof(buf), "IP Address: %s", m_settings.ipAddress);
    lv_obj_t *ip_label = lv_label_create(cont);
    lv_label_set_text(ip_label, buf);

    lv_obj_t *conn_label = lv_label_create(cont);
    lv_label_set_text(conn_label, m_settings.wifiConnected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(conn_label,
                                m_settings.wifiConnected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);

    lv_obj_t *scan_btn = lv_btn_create(cont);
    lv_obj_set_size(scan_btn, 150, 40);
    lv_obj_t *scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan WiFi");
    lv_obj_center(scan_label);

    lv_obj_t *hotspot_btn = lv_btn_create(cont);
    lv_obj_set_size(hotspot_btn, 150, 40);
    lv_obj_t *hotspot_label = lv_label_create(hotspot_btn);
    lv_label_set_text(hotspot_label, "Enable Hotspot");
    lv_obj_center(hotspot_label);

    m_subpage = cont;
}

void SettingsUi::createBluetoothPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 15);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Bluetooth");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    lv_obj_t *status_label = lv_label_create(cont);
    lv_label_set_text(status_label, m_settings.bluetoothEnabled ? "Enabled" : "Disabled");
    lv_obj_set_style_text_color(status_label,
                                m_settings.bluetoothEnabled ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);

    char buf[64];
    snprintf(buf, sizeof(buf), "Device Name: %s", m_settings.bluetoothDeviceName);
    lv_obj_t *device_label = lv_label_create(cont);
    lv_label_set_text(device_label, buf);

    lv_obj_t *toggle_btn = lv_btn_create(cont);
    lv_obj_set_size(toggle_btn, 150, 40);
    lv_obj_t *toggle_label = lv_label_create(toggle_btn);
    lv_label_set_text(toggle_label, m_settings.bluetoothEnabled ? "Disable" : "Enable");
    lv_obj_center(toggle_label);

    lv_obj_t *search_btn = lv_btn_create(cont);
    lv_obj_set_size(search_btn, 150, 40);
    lv_obj_t *search_label = lv_label_create(search_btn);
    lv_label_set_text(search_label, "Search Devices");
    lv_obj_center(search_label);

    lv_obj_t *send_btn = lv_btn_create(cont);
    lv_obj_set_size(send_btn, 150, 40);
    lv_obj_t *send_label = lv_label_create(send_btn);
    lv_label_set_text(send_label, "Send File");
    lv_obj_center(send_label);

    m_subpage = cont;
}

void SettingsUi::volumeSliderCb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    SettingsUi::instance().m_settings.volume = lv_slider_get_value(slider);
}

void SettingsUi::createSoundPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 20);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Sound");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "Volume: %d%%", m_settings.volume);
    lv_obj_t *volume_label = lv_label_create(cont);
    lv_label_set_text(volume_label, buf);

    lv_obj_t *slider = lv_slider_create(cont);
    lv_obj_set_size(slider, 200, 20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, m_settings.volume, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, volumeSliderCb, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *mute_btn = lv_btn_create(cont);
    lv_obj_set_size(mute_btn, 100, 40);
    lv_obj_t *mute_label = lv_label_create(mute_btn);
    lv_label_set_text(mute_label, "Mute");
    lv_obj_center(mute_label);

    m_subpage = cont;
}

void SettingsUi::brightnessSliderCb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    SettingsUi::instance().m_settings.brightness = lv_slider_get_value(slider);
}

void SettingsUi::createDisplayPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 20);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Display");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d%%", m_settings.brightness);
    lv_obj_t *brightness_label = lv_label_create(cont);
    lv_label_set_text(brightness_label, buf);

    lv_obj_t *slider = lv_slider_create(cont);
    lv_obj_set_size(slider, 200, 20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, m_settings.brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightnessSliderCb, LV_EVENT_VALUE_CHANGED, nullptr);

    m_subpage = cont;
}

void SettingsUi::sensitivitySliderCb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    SettingsUi::instance().m_settings.keySensitivity = lv_slider_get_value(slider);
}

void SettingsUi::createKeyboardPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 20);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Keyboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "Sensitivity: %d", m_settings.keySensitivity);
    lv_obj_t *sensitivity_label = lv_label_create(cont);
    lv_label_set_text(sensitivity_label, buf);

    lv_obj_t *slider = lv_slider_create(cont);
    lv_obj_set_size(slider, 200, 20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, m_settings.keySensitivity, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, sensitivitySliderCb, LV_EVENT_VALUE_CHANGED, nullptr);

    m_subpage = cont;
}

void SettingsUi::layoutBtnClickCb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    DesktopLayout layout = (DesktopLayout)(intptr_t)lv_obj_get_user_data(btn);
    UiManager::instance().setLayout(layout);
}

void SettingsUi::createDesktopPage(lv_obj_t *parent)
{
    lv_obj_t *cont = createSubpageContainer(parent, 20);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Desktop Layout");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    static const char *layout_names[] = {"2x2", "2x3", "3x3", "4x2", "4x3"};
    static const DesktopLayout layouts[] = {
        DesktopLayout::LAYOUT_2x2, DesktopLayout::LAYOUT_2x3, DesktopLayout::LAYOUT_3x3,
        DesktopLayout::LAYOUT_4x2, DesktopLayout::LAYOUT_4x3
    };

    for (size_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); i++) {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_size(btn, 120, 40);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, layout_names[i]);
        lv_obj_center(label);

        /* 布局枚举直存指针值，无需 malloc（修复原实现泄漏） */
        lv_obj_set_user_data(btn, (void *)(intptr_t)layouts[i]);
        lv_obj_add_event_cb(btn, layoutBtnClickCb, LV_EVENT_CLICKED, nullptr);

        if (UiManager::instance().layout() == layouts[i]) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        }
    }

    m_subpage = cont;
}

void SettingsUi::menuItemClickCb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    SettingsUi *self = (SettingsUi *)lv_event_get_user_data(e);

    /* 先取出 page id 再删容器（修复原实现先删后读的 use-after-free） */
    int page_id = (int)(intptr_t)lv_obj_get_user_data(btn);

    if (self->m_menu != nullptr) {
        lv_obj_del(self->m_menu);
        self->m_menu = nullptr;
    }

    switch (page_id) {
        case 0: self->createSystemInfoPage(self->m_page); break;
        case 1: self->createNetworkPage(self->m_page); break;
        case 2: self->createBluetoothPage(self->m_page); break;
        case 3: self->createSoundPage(self->m_page); break;
        case 4: self->createDisplayPage(self->m_page); break;
        case 5: self->createKeyboardPage(self->m_page); break;
        case 6: self->createDesktopPage(self->m_page); break;
        case 7: self->resetToDefaults(); break;
        default: break;
    }
}

void SettingsUi::showMenu()
{
    lv_obj_t *menu_cont = lv_obj_create(m_page);
    lv_obj_set_size(menu_cont, LV_HOR_RES - 40, LV_VER_RES - 100);
    lv_obj_align(menu_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(menu_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(menu_cont, 10, 0);

    static const char *menu_items[] = {
        "System Info", "Network", "Bluetooth", "Sound",
        "Display", "Keyboard", "Desktop", "Reset to Defaults",
    };
    static const char *menu_icons[] = {
        LV_SYMBOL_LEFT, LV_SYMBOL_WIFI, LV_SYMBOL_BLUETOOTH, LV_SYMBOL_AUDIO,
        LV_SYMBOL_PREV, LV_SYMBOL_KEYBOARD, LV_SYMBOL_HOME, LV_SYMBOL_REFRESH,
    };
    const int menu_count = (int)(sizeof(menu_items) / sizeof(menu_items[0]));

    for (int i = 0; i < menu_count; i++) {
        lv_obj_t *btn = lv_btn_create(menu_cont);
        lv_obj_set_size(btn, LV_HOR_RES - 80, 50);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(btn, 15, 0);

        lv_obj_t *icon_label = lv_label_create(btn);
        lv_label_set_text(icon_label, menu_icons[i]);
        lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_14, 0);

        lv_obj_t *text_label = lv_label_create(btn);
        lv_label_set_text(text_label, menu_items[i]);
        lv_obj_set_style_text_font(text_label, &lv_font_montserrat_14, 0);

        /* 菜单序号直存指针值，无需 malloc（修复原实现泄漏） */
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, menuItemClickCb, LV_EVENT_CLICKED, this);
    }

    m_menu = menu_cont;
    m_subpage = nullptr;
}

void SettingsUi::setVersion(const char *projectVersion, const char *componentVersion)
{
    if (projectVersion != nullptr) {
        safeCopy(m_settings.projectVersion, sizeof(m_settings.projectVersion), projectVersion);
    }
    if (componentVersion != nullptr) {
        safeCopy(m_settings.componentVersion, sizeof(m_settings.componentVersion), componentVersion);
    }
}

void SettingsUi::updateWifiStatus(const char *ssid, const char *ipAddress, int signalLevel, bool connected)
{
    if (ssid != nullptr) {
        safeCopy(m_settings.ssid, sizeof(m_settings.ssid), ssid);
    }
    if (ipAddress != nullptr) {
        safeCopy(m_settings.ipAddress, sizeof(m_settings.ipAddress), ipAddress);
    }
    m_settings.wifiSignalLevel = signalLevel;
    m_settings.wifiConnected = connected;
}

void SettingsUi::updateBluetoothStatus(bool enabled, const char *deviceName)
{
    m_settings.bluetoothEnabled = enabled;
    if (deviceName != nullptr) {
        safeCopy(m_settings.bluetoothDeviceName, sizeof(m_settings.bluetoothDeviceName), deviceName);
    }
}

void SettingsUi::setVolume(int volume)
{
    m_settings.volume = clampPercent(volume);
}

void SettingsUi::setBrightness(int brightness)
{
    m_settings.brightness = clampPercent(brightness);
}

void SettingsUi::setKeySensitivity(int sensitivity)
{
    m_settings.keySensitivity = clampPercent(sensitivity);
}

void SettingsUi::resetToDefaults()
{
    safeCopy(m_settings.projectVersion, sizeof(m_settings.projectVersion), "v1.0.0");
    safeCopy(m_settings.componentVersion, sizeof(m_settings.componentVersion), SETTINGS_VERSION);
    safeCopy(m_settings.ipAddress, sizeof(m_settings.ipAddress), "0.0.0.0");
    safeCopy(m_settings.ssid, sizeof(m_settings.ssid), "Not connected");
    m_settings.wifiSignalLevel = 0;
    m_settings.wifiConnected = false;
    m_settings.bluetoothEnabled = false;
    safeCopy(m_settings.bluetoothDeviceName, sizeof(m_settings.bluetoothDeviceName), "Unknown");
    m_settings.volume = 50;
    m_settings.brightness = 80;
    m_settings.keySensitivity = 50;

    /* 页面显示中：整页重建 */
    if (m_page != nullptr) {
        m_menu = nullptr;
        m_subpage = nullptr;
        lv_obj_clean(m_page);

        createBackBtn(m_page);

        lv_obj_t *title = lv_label_create(m_page);
        lv_label_set_text(title, "Settings");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

        showMenu();
    }
}
