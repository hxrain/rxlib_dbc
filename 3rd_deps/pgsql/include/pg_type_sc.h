#ifndef _PG_TYPE_SC_H_
#define _PG_TYPE_SC_H_

//from pgsql source code : /src/include/catalog/pg_type.h

//---------------------------------------------------------
//select 123.01::int2,123.01::int4,123.01::int8,3898765432::oid,123.01::float4,123.01::float8,123.01::NUMERIC
//DESCR("-32 thousand to 32 thousand, 2-byte storage");
const int PG_TYPE_INT2_OID              = 21;               //int16_t
//DESCR("-2 billion to 2 billion integer, 4-byte storage");
const int PG_TYPE_INT4_OID              = 23;               //int32_t
//DESCR("~18 digit integer, 8-byte storage");
const int PG_TYPE_INT8_OID              = 20;               //int64_t
//DESCR("object identifier(oid), maximum 4 billion");
const int PG_TYPE_OID_OID               = 26;               //uint32_t
//DESCR("single-precision floating point number, 4-byte storage");
const int PG_TYPE_FLOAT4OID             = 700;              //float
//DESCR("double-precision floating point number, 8-byte storage");
const int PG_TYPE_FLOAT8OID             = 701;              //double
//DESCR("numeric(precision, decimal), arbitrary precision number");
const int PG_TYPE_NUMERIC_OID           = 1700;             //

//---------------------------------------------------------
//DESCR("variable-length string, binary values escaped");
const int PG_TYPE_BYTEA_OID             = 17;
//DESCR("single character");
const int PG_TYPE_CHAR_OID              = 18;               //=BPCHAROID
//DESCR("63-byte type for storing system identifiers");
const int PG_TYPE_NAME_OID              = 19;
//DESCR("variable-length string, no limit specified");
const int PG_TYPE_TEXT_OID              = 25;
//DESCR("char(length), blank-padded string, fixed storage length");
const int PG_TYPE_BPCHAR_OID            = 1042;
//DESCR("varchar(length), non-blank-padded string, variable storage length");
const int PG_TYPE_VARCHAR_OID           = 1043;

//---------------------------------------------------------
//SELECT  NOW()::date,NOW()::time,NOW()::abstime,NOW()::timestamp,NOW()::timestamptz
//DESCR("date");
const int PG_TYPE_DATE_OID              = 1082;             //2018-12-12
//DESCR("time of day");
const int PG_TYPE_TIME_OID              = 1083;             //02:40:37.388487
//DESCR("absolute, limited-range date and time (Unix system time)");
const int PG_TYPE_ABSTIME_OID           = 702;              //2018-12-12 02:40:37+00
//DESCR("date and time");
const int PG_TYPE_TIMESTAMP_OID         = 1114;             //2018-12-12 02:40:37.388487
//DESCR("date and time with time zone");
const int PG_TYPE_TIMESTAMPTZ_OID       = 1184;             //2018-12-12 02:40:37.388487+00



#endif