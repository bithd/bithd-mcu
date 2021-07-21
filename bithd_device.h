#ifndef BITHD_DEVICE_H

#if defined(BITHD_RAZOR)
#define IF_RAZOR(...) __VA_ARGS__
#define IF_BITHD(...)
#elif defined(BITHD_BITHD)
#define IF_RAZOR(...)
#define IF_BITHD(...) __VA_ARGS__ 
#else
#error "No valid DEVICE_MODEL defined"
#endif

#endif // BITHD_DEVICE_H
