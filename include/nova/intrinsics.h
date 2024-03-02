/**
 * Part of Nova C++ Library.
 * 
 * Compiler magics, macros, instrinsics etc...
 */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  #define NOVA_WIN
#elif defined(__linux__)
  #define NOVA_LINUX
#elif defined(__APPLE__)
  #define NOVA_MACOS
#endif