#ifndef SNOWROBOT_REMOTECONTROL_COMMON_GST_WRAPPERS_H
#define SNOWROBOT_REMOTECONTROL_COMMON_GST_WRAPPERS_H



#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <new>


#include <gst/gst.h>

namespace snowrobot {


// This file contains some c++ wrappers that make the gstreamer c api slightly less annoying to work with, by
// automatic freeing of resources with std::unique_ptr.

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

auto unrefGstCaps = [](GstCaps* the_caps) {gst_caps_unref(the_caps);};
using GstCaps_ptr = std::unique_ptr<GstCaps, decltype(unrefGstCaps)>;
GstCaps_ptr make_GstCaps_ptr(GstCaps* the_caps) {
  return GstCaps_ptr(the_caps, unrefGstCaps);
}

auto unrefGstElement = [](GstElement* obj) {gst_object_unref(obj);};
using GstElement_ptr = std::unique_ptr<GstElement, decltype(unrefGstElement)>;
GstElement_ptr make_GstElement_ptr(GstElement* obj) {
  return GstElement_ptr(obj, unrefGstElement);
}

 


#define ASSERT_NOT_NULL(the_pointer) while(true) {\
  if(!the_pointer) { \
    std::ostringstream msg; \
    msg << "Got a nullptr at " << __FILE__ << ":" << __LINE__ << "!"; \
    throw std::runtime_error(msg.str()); \
  } \
  break;}

#define ASSERT_TRUE(the_test_variable) while(true) {\
  if(!the_test_variable) { \
    std::ostringstream msg; \
    msg << "Got a falsish expression at " << __FILE__ << ":" << __LINE__ << "!"; \
    throw std::runtime_error(msg.str()); \
  } \
  break;}



// The function_pointer magic was copied from here: https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer
template<int, typename Callable, typename Ret, typename... Args>
auto function_pointer_(Callable&& c, Ret (*)(Args...))
{
    static std::decay_t<Callable> storage = std::forward<Callable>(c);
    static bool used = false;
    if(used)
    {
        using type = decltype(storage);
        storage.~type();
        new (&storage) type(std::forward<Callable>(c));
    }
    used = true;

    return [](Args... args) -> Ret {
        auto& c = *std::launder(&storage);
        return Ret(c(std::forward<Args>(args)...));
    };
}

template<typename Fn, int N = 0, typename Callable>

Fn* function_pointer(Callable&& c)
{
    return function_pointer_<N>(std::forward<Callable>(c), (Fn*)nullptr);
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
