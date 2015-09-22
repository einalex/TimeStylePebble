#include <pebble.h>
#include <math.h>
#include "settings.h"
#include "languages.h"
#include "sidebar_widgets.h"

bool SidebarWidgets_useCompactMode = false;

// sidebar icons
#ifdef PBL_COLOR
  GDrawCommandImage* dateImage;
  GDrawCommandImage* disconnectImage;
  GDrawCommandImage* batteryImage;
  GDrawCommandImage* batteryChargeImage;
#else
  GBitmap* dateImage;
  GBitmap* disconnectImage;
  GBitmap* batteryImage;
  GBitmap* batteryChargeImage;
#endif

// fonts
GFont smSidebarFont;
GFont mdSidebarFont;
GFont lgSidebarFont;
GFont currentSidebarFont;
GFont batteryFont;

// the date and weather strings
char currentDayName[8];
char currentDayNum[8];
char currentMonth[8];

// the widgets
SidebarWidget batteryMeterWidget;
void BatteryMeter_draw(GContext* ctx, int yPosition);
int BatteryMeter_getHeight();

SidebarWidget emptyWidget;
void EmptyWidget_draw(GContext* ctx, int yPosition);
int EmptyWidget_getHeight();

SidebarWidget dateWidget;
void DateWidget_draw(GContext* ctx, int yPosition);
int DateWidget_getHeight();

void SidebarWidgets_init() {
  // load fonts
  smSidebarFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  mdSidebarFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  lgSidebarFont = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  // load the sidebar graphics
  #ifdef PBL_COLOR
    dateImage = gdraw_command_image_create_with_resource(RESOURCE_ID_DATE_BG);
    disconnectImage = gdraw_command_image_create_with_resource(RESOURCE_ID_DISCONNECTED);
    batteryImage = gdraw_command_image_create_with_resource(RESOURCE_ID_BATTERY_BG);
    batteryChargeImage = gdraw_command_image_create_with_resource(RESOURCE_ID_BATTERY_CHARGE);
  #else
    dateImage = gbitmap_create_with_resource(RESOURCE_ID_DATE_BG);
    disconnectImage = gbitmap_create_with_resource(RESOURCE_ID_DISCONNECTED);
    batteryImage = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BG);
    batteryChargeImage = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE);
  #endif

  SidebarWidgets_updateFonts();

  // set up widgets' function pointers correctly
  batteryMeterWidget.getHeight = BatteryMeter_getHeight;
  batteryMeterWidget.draw      = BatteryMeter_draw;

  emptyWidget.getHeight = EmptyWidget_getHeight;
  emptyWidget.draw      = EmptyWidget_draw;

  dateWidget.getHeight = DateWidget_getHeight;
  dateWidget.draw      = DateWidget_draw;
}

void SidebarWidgets_deinit() {
  #ifdef PBL_COLOR
    gdraw_command_image_destroy(dateImage);
    gdraw_command_image_destroy(disconnectImage);
    gdraw_command_image_destroy(batteryImage);
    gdraw_command_image_destroy(batteryChargeImage);
  #else
    gbitmap_destroy(dateImage);
    gbitmap_destroy(disconnectImage);
    gbitmap_destroy(batteryImage);
    gbitmap_destroy(batteryChargeImage);
  #endif
}

void SidebarWidgets_updateFonts() {
  if(globalSettings.useLargeFonts) {
    currentSidebarFont = lgSidebarFont;
    batteryFont = lgSidebarFont;
  } else {
    currentSidebarFont = mdSidebarFont;
    batteryFont = smSidebarFont;
  }
}

void SidebarWidgets_updateTime(struct tm* timeInfo) {
  // set all the date strings
  strftime(currentDayNum,  3, "%e", timeInfo);

  strncpy(currentDayName, dayNames[globalSettings.languageId][timeInfo->tm_wday], sizeof(currentDayName));
  strncpy(currentMonth, monthNames[globalSettings.languageId][timeInfo->tm_mon], sizeof(currentMonth));

  // remove padding on date num, if needed
  if(currentDayNum[0] == ' ') {
    currentDayNum[0] = currentDayNum[1];
    currentDayNum[1] = '\0';
  }
}

/* Sidebar Widget Selection */
SidebarWidget getSidebarWidgetByType(SidebarWidgetType type) {
  switch(type) {
    case BATTERY_METER:
      return batteryMeterWidget;
      break;
    case DATE:
      return dateWidget;
      break;
    default:
      return emptyWidget;
      break;
  }
}

/********** functions for the empty widget **********/
int EmptyWidget_getHeight() {
  return 0;
}

void EmptyWidget_draw(GContext* ctx, int yPosition) {
  return;
}

/********** functions for the battery meter widget **********/

int BatteryMeter_getHeight() {
  return 20; // TODO: Make this more real
}

void BatteryMeter_draw(GContext* ctx, int yPosition) {
  BatteryChargeState chargeState = battery_state_service_peek();

  char batteryString[6];

  int batteryPositionY = yPosition;

  if(chargeState.is_charging) {
    if(batteryChargeImage) {
      #ifdef PBL_COLOR
        gdraw_command_image_draw(ctx, batteryChargeImage, GPoint(3, batteryPositionY));
      #else
        graphics_draw_bitmap_in_rect(ctx, batteryChargeImage, GRect(3, batteryPositionY, 25, 25));
      #endif
    }
  } else {
    if (batteryImage) {
      #ifdef PBL_COLOR
        gdraw_command_image_draw(ctx, batteryImage, GPoint(3, batteryPositionY));
      #else
        graphics_draw_bitmap_in_rect(ctx, batteryImage, GRect(3, batteryPositionY, 25, 25));
      #endif
    }

    int width = roundf(18 * chargeState.charge_percent / 100.0f);

    graphics_context_set_fill_color(ctx, GColorBlack);

    #ifdef PBL_COLOR
      if(chargeState.charge_percent <= 20) {
        graphics_context_set_fill_color(ctx, GColorRed);
      }
    #else
      if(globalSettings.sidebarTextColor == GColorWhite) {
        graphics_context_set_fill_color(ctx, GColorWhite);
      }
    #endif

    graphics_fill_rect(ctx, GRect(6, 8 + batteryPositionY, width, 8), 0, GCornerNone);
  }

  // never show battery % while charging, because of this issue:
  // https://github.com/freakified/TimeStylePebble/issues/11
  if(globalSettings.showBatteryPct && !chargeState.is_charging) {
    if(!globalSettings.useLargeFonts) {
      snprintf(batteryString, sizeof(batteryString), "%d%%", chargeState.charge_percent);

      graphics_draw_text(ctx,
                         batteryString,
                         batteryFont,
                         GRect(-4, 18 + batteryPositionY, 38, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);
    } else {
      snprintf(batteryString, sizeof(batteryString), "%d", chargeState.charge_percent);

      graphics_draw_text(ctx,
                         batteryString,
                         batteryFont,
                         GRect(-4, 14 + batteryPositionY, 38, 20),
                         GTextOverflowModeFill,
                         GTextAlignmentCenter,
                         NULL);
    }
  }
}

/********** date widget **********/

int DateWidget_getHeight() {
  return 50; // TODO: Make this more real
}

void DateWidget_draw(GContext* ctx, int yPosition) {
  // TODO: made this use the appropriate y Positions

  // in large font mode, draw a different date image
  if(!globalSettings.useLargeFonts) {
    if (dateImage) {
      #ifdef PBL_COLOR
        gdraw_command_image_draw(ctx, dateImage, GPoint(3, 118));
      #else
        graphics_draw_bitmap_in_rect(ctx, dateImage, GRect(3, 118, 25, 25));
      #endif
    }
  } else {
    #ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(2, 119, 26, 22), 2, GCornersAll);
    #else
      if(globalSettings.sidebarTextColor == GColorWhite) {
        graphics_context_set_fill_color(ctx, GColorWhite);
      } else {
        graphics_context_set_fill_color(ctx, GColorBlack);
      }

      graphics_fill_rect(ctx, GRect(1, 119, 28, 22), 2, GCornersAll);

      if(globalSettings.sidebarTextColor == GColorWhite) {
        graphics_context_set_fill_color(ctx, GColorBlack);
      } else {
        graphics_context_set_fill_color(ctx, GColorWhite);
      }

      graphics_fill_rect(ctx, GRect(3, 121, 24, 18), 0, GCornersAll);


    #endif

  }

  // color pebble should always use black for the date number...
  #ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, GColorBlack);
  #endif

  int yPos = 0;

  yPos = globalSettings.useLargeFonts ? 113 : 121;

  graphics_draw_text(ctx,
                     currentDayNum,
                     currentSidebarFont,
                     GRect(0, yPos, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);


   // switch back to normal color for the rest
  #ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, globalSettings.sidebarTextColor);
  #endif

  yPos = globalSettings.useLargeFonts ? 89 : 95;

  // now draw in the date info
  graphics_draw_text(ctx,
                     currentDayName,
                     currentSidebarFont,
                     GRect(-5, yPos, 40, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  // y position for month text
  yPos = globalSettings.useLargeFonts ? 137 : 142;

  graphics_draw_text(ctx,
                     currentMonth,
                     currentSidebarFont,
                     GRect(0, yPos, 30, 20),
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
}
