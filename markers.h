#ifndef ANCOR_PROGRAMM_TRAITS_MARKERS_H
#define ANCOR_PROGRAMM_TRAITS_MARKERS_H

#define MARKER_INTEGER_NAME_PREFIX marker_integer_

// use this macro
#define marker(x) int CONCAT(MARKER_INTEGER_NAME_PREFIX,x);

//  needed for correct macro expansion
#define CONCAT(x,y) CONCAT2(x,y)
#define CONCAT2(x,y) x##y


//TODO these could be made internal to our library
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define MARKER_INTEGER_NAME_PREFIX_STR STRINGIFY(MARKER_INTEGER_NAME_PREFIX)

#endif //ANCOR_PROGRAMM_TRAITS_MARKERS_H
