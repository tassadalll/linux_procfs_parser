#ifndef __PP_INTERNAL__
#define __PP_INTERNAL__

#define ERRGOTO( _ret, _label ) \
    _ret = false;               \
    goto _label;

#define UNKNOWN_INODE 0

#endif
