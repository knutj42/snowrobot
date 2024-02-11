#ifndef SNOWROBOT_REMOTECONTROL_COMMON_GST_WRAPPERS_H
#define SNOWROBOT_REMOTECONTROL_COMMON_GST_WRAPPERS_H

// This file contains some c++ wrappers that make the gstreamer c api slightly less annoying to work with, by
// automatic freeing of resources with std::unique_ptr.


#include <memory>
#include <string>
#include <gst/gst.h>

namespace snowrobot {


std::string string_from_gchar(gchar* gchar_ptr) {
  std::string result(gchar_ptr);
  g_free(gchar_ptr);
  return result;
}

auto freeGList = [](GList* the_list) {g_list_free(the_list);};
using GList_ptr = std::unique_ptr<GList, decltype(freeGList)>;
GList_ptr make_GList_ptr(GList* the_list) {
  return GList_ptr(the_list, freeGList);
}

auto freeGstCaps = [](GstCaps* the_caps) {gst_caps_unref(the_caps);};
using GstCaps_ptr = std::unique_ptr<GstCaps, decltype(freeGstCaps)>;
GstCaps_ptr make_GstCaps_ptr(GstCaps* the_caps) {
  return GstCaps_ptr(the_caps, freeGstCaps);
}

auto stopAndFreeMonitor = [](GstDeviceMonitor* monitor) {
  gst_device_monitor_stop(monitor);
  gst_object_unref(monitor);
};
using GstDeviceMonitor_ptr = std::unique_ptr<GstDeviceMonitor, decltype(stopAndFreeMonitor)>;
GstDeviceMonitor_ptr make_GstDeviceMonitor_ptr(GstDeviceMonitor* monitor) {
  return GstDeviceMonitor_ptr(monitor, stopAndFreeMonitor);
}

}

#endif
