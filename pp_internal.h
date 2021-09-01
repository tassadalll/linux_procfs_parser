#ifndef __PP_INTERNAL__
#define __PP_INTERNAL__

#define SETERRGOTO( _ret, _label ) \
    _ret = false;                  \
    goto _label;

#define IFERRGOTO( _ret, _label ) \
    if( _ret == false ) {         \
        goto _label;              \
    }

#define NULLERRGOTO( _var, _ret, _label ) \
    if( _var == NULL ) {                  \
        _ret = false;                     \
        goto _label;                      \
    }

#define UNKNOWN_INODE 0

#endif
