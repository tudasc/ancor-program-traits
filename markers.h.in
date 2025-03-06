#ifndef ANCOR_PROGRAMM_TRAITS_MARKERS_H
#define ANCOR_PROGRAMM_TRAITS_MARKERS_H

// settings
// set type of marker to use
#define USE_MARKER_INTEGER
//#define USE_MARKER_FUNCTION

#define MARKER_NAME_PREFIX marker_symbol_trait_

#ifdef USE_MARKER_INTEGER
#ifdef USE_MARKER_FUNCTION
_Static_assert(0, "Use either setting, not both");
#endif
#endif


#ifdef USE_MARKER_INTEGER
#ifdef __GNUC__  // GCC/Clang specific: prevent compiler form optimizing away the marker
#define ATTRS __attribute__((used,visibility("default"))) inline
#else
#define ATTRS
#endif


#define marker(x)  ATTRS int CONCAT(MARKER_NAME_PREFIX,x);

//this does always lead to a compiler warning about inline, which unfortunately cannot be suppressed by a pragma
#endif

#ifdef USE_MARKER_FUNCTION

#define marker(x)  __attribute__((weak)) void CONCAT(MARKER_NAME_PREFIX,x)();\
__attribute__((constructor)) static void CONCAT(CONCAT(MARKER_NAME_PREFIX,constructor_helper_),x)(){\
    if (CONCAT(MARKER_NAME_PREFIX,x)) CONCAT(MARKER_NAME_PREFIX,x)();\
}

#endif

//  needed for correct macro expansion
#define CONCAT(x,y) CONCAT2(x,y)
#define CONCAT2(x,y) x##y

//TODO these could be made internal to our library
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define MARKER_NAME_PREFIX_STR STRINGIFY(MARKER_NAME_PREFIX)

#endif //ANCOR_PROGRAMM_TRAITS_MARKERS_H
