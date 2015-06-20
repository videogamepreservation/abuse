dnl AB_CHECK_FLAG(FLAG, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(AB_CHECK_FLAG,
[
AC_MSG_CHECKING([whether ifelse(AC_LANG, CPLUSPLUS, ${CXX}, ${CC}) supports the $1 flag])
ab_flag_var=`echo $1 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ab_cv_flag_$ab_flag_var,
[
ifelse(AC_LANG, CPLUSPLUS, [
ab_save_flags="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $1"
], [
ab_save_flags="$CFLAGS"
CFLAGS="$CFLAGS $1"
])dnl
AC_TRY_COMPILE(,[int a;],
eval "ab_cv_flag_$ab_flag_var=yes", eval "ab_cv_flag_$ab_flag_var=no")
ifelse(AC_LANG, CPLUSPLUS, CXXFLAGS="$ab_save_flags", CFLAGS="$ab_save_flags")
])
if eval "test \"`echo '$ab_cv_flag_'$ab_flag_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi
])

dnl AB_ADD_FLAGS(FLAG...)
AC_DEFUN(AB_ADD_FLAGS,
[ab_flags=
for ab_flag in $1; do
AB_CHECK_FLAG([$ab_flag], [ab_flags="$ab_flags $ab_flag"])
ifelse(AC_LANG, CPLUSPLUS, [
ADDCXXFLAGS="$ab_flags"
AC_SUBST(ADDCXXFLAGS)
], [
ADDCFLAGS="$ab_flags"
AC_SUBST(ADDCFLAGS)
])dnl
done
])

AC_DEFUN(AB_C_BIGENDIAN,
[AC_CACHE_CHECK(whether byte ordering is bigendian, ac_cv_c_bigendian,
[ac_cv_c_bigendian=unknown
# See if we have a good endian.h.
AC_TRY_CPP([#include <endian.h>], [AC_TRY_COMPILE([#include <endian.h>], [
#if !__BYTE_ORDER || !__BIG_ENDIAN
  bogus endian macros
#endif], [AC_TRY_COMPILE([#include <endian.h>], [
#if __BYTE_ORDER != __BIG_ENDIAN
 not big endian
#endif], ac_cv_c_bigendian=yes, ac_cv_c_bigendian=no)])])
if test $ac_cv_c_bigendian = unknown; then
# See if sys/param.h defines the BYTE_ORDER macro.
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/param.h>], [
#if !BYTE_ORDER || !BIG_ENDIAN || !LITTLE_ENDIAN
 bogus endian macros
#endif], [# It does; now see whether it defined to BIG_ENDIAN or not.
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/param.h>], [
#if BYTE_ORDER != BIG_ENDIAN
 not big endian
#endif], ac_cv_c_bigendian=yes, ac_cv_c_bigendian=no)])
fi
if test $ac_cv_c_bigendian = unknown; then
AC_TRY_RUN([main () {
  /* Are we little or big endian?  From Harbison&Steele.  */
  union
  {
    long l;
    char c[sizeof (long)];
  } u;
  u.l = 1;
  exit (u.c[sizeof (long) - 1] == 1);
}], ac_cv_c_bigendian=no, ac_cv_c_bigendian=yes)
fi])
if test $ac_cv_c_bigendian = yes; then
  AC_DEFINE(WORDS_BIGENDIAN)
fi
])
