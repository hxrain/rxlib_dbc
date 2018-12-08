#ifndef _PG_TYPE_SC_H_
#define _PG_TYPE_SC_H_

//from pgsql source code : /src/include/catalog/pg_type.h

 /*
  * macros for values of poor-mans-enumerated-type columns
  */
 #define  TYPTYPE_BASE       'b' /* base type (ordinary scalar type) */
 #define  TYPTYPE_COMPOSITE  'c' /* composite (e.g., table's rowtype) */
 #define  TYPTYPE_DOMAIN     'd' /* domain over another type */
 #define  TYPTYPE_ENUM       'e' /* enumerated type */
 #define  TYPTYPE_PSEUDO     'p' /* pseudo-type */
 #define  TYPTYPE_RANGE      'r' /* range type */
 
 #define  TYPCATEGORY_INVALID    '\0'    /* not an allowed category */
 #define  TYPCATEGORY_ARRAY      'A'
 #define  TYPCATEGORY_BOOLEAN    'B'
 #define  TYPCATEGORY_COMPOSITE  'C'
 #define  TYPCATEGORY_ENUM       'E'
 #define  TYPCATEGORY_GEOMETRIC  'G'
 #define  TYPCATEGORY_NETWORK    'I' /* think INET */
 #define  TYPCATEGORY_PSEUDOTYPE 'P'
 #define  TYPCATEGORY_RANGE      'R'
 #define  TYPCATEGORY_TIMESPAN   'T'
 #define  TYPCATEGORY_USER       'U'
 #define  TYPCATEGORY_BITSTRING  'V' /* er ... "varbit"? */
 #define  TYPCATEGORY_UNKNOWN    'X'

 #define  TYPCATEGORY_DATETIME   'D'
 #define  TYPCATEGORY_NUMERIC    'N'
 #define  TYPCATEGORY_STRING     'S'

#endif