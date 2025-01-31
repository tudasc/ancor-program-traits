#ifndef ANCOR_PROGRAMM_TRAITS_MARKERS_H
#define ANCOR_PROGRAMM_TRAITS_MARKERS_H

#ifdef __GNUC__  // GCC/Clang specific: prevent compiler form optimizing away the marker
#define ATTRS __attribute__((used,visibility("default"))) inline
#else
#define ATTRS
#endif
//TODO this does always lead to a compiler warning about inline, which unfortunately cannot be suppressed by a pragma

#define MARKER_INTEGER_NAME_PREFIX marker_integer_
// from markers.h:
#define marker(x)  ATTRS int CONCAT(MARKER_INTEGER_NAME_PREFIX,x);


//  needed for correct macro expansion
#define CONCAT(x,y) CONCAT2(x,y)
#define CONCAT2(x,y) x##y

// x must have weak linkage
#define call_if_present(x) if (x) x()


//TODO these could be made internal to our library
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define MARKER_INTEGER_NAME_PREFIX_STR STRINGIFY(MARKER_INTEGER_NAME_PREFIX)

#endif //ANCOR_PROGRAMM_TRAITS_MARKERS_H
