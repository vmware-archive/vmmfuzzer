AC_INIT([vmmfuzzer], [0.0.1], [bug@vmmfuzzer.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

# Checks for libraries.
AC_CHECK_LIB([m], [pow])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memmove pow strdup strtoul strtoull])

AC_CANONICAL_HOST
AS_CASE([$host_cpu],
 [i?86], [host_cpu = i386])

AC_CONFIG_FILES([
 Makefile
 lib/Makefile
 src/Makefile
])
AC_OUTPUT
