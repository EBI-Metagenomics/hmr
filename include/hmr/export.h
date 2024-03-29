#ifndef HMR_API_H
#define HMR_API_H

/* clang-format off */
#ifdef HMR_STATIC_DEFINE
#  define HMR_API
#else
#  ifdef hmr_EXPORTS /* We are building this library */
#    ifdef _WIN32
#      define HMR_API __declspec(dllexport)
#    else
#      define HMR_API __attribute__((visibility("default")))
#    endif
#  else /* We are using this library */
#    ifdef _WIN32
#      define HMR_API __declspec(dllimport)
#    else
#      define HMR_API __attribute__((visibility("default")))
#    endif
#  endif
#endif
/* clang-format on */

#endif
