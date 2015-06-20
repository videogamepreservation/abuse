dnl aclocal.m4 generated automatically by aclocal 1.2

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

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AM_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION"))
AM_SANITY_CHECK
AC_ARG_PROGRAM
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_PROG_MAKE_SET])


# serial 1

AC_DEFUN(AM_PROG_INSTALL,
[AC_REQUIRE([AC_PROG_INSTALL])
test -z "$INSTALL_SCRIPT" && INSTALL_SCRIPT='${INSTALL_PROGRAM}'
AC_SUBST(INSTALL_SCRIPT)dnl
])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "$@" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_WARN([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

