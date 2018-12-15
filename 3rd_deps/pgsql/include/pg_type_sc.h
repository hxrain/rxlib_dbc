#ifndef _PG_TYPE_SC_H_
#define _PG_TYPE_SC_H_

//from pgsql source code : /src/include/catalog/pg_type.h

const int PG_DATA_TYPE_UNKNOW            = 0;
//---------------------------------------------------------
//select 123.01::int2,123.01::int4,123.01::int8,3898765432::oid,123.01::float4,123.01::float8,123.01::NUMERIC
//DESCR("-32 thousand to 32 thousand, 2-byte storage");
const int PG_DATA_TYPE_INT2              = 21;              //int16_t
//DESCR("-2 billion to 2 billion integer, 4-byte storage"); 
const int PG_DATA_TYPE_INT4              = 23;              //int32_t
//DESCR("~18 digit integer, 8-byte storage");
const int PG_DATA_TYPE_INT8              = 20;              //int64_t
//DESCR("object identifier(oid), maximum 4 billion");
const int PG_DATA_TYPE_OID               = 26;              //uint32_t
//DESCR("single-precision floating point number, 4-byte storage");
const int PG_DATA_TYPE_FLOAT4            = 700;             //float
//DESCR("double-precision floating point number, 8-byte storage");
const int PG_DATA_TYPE_FLOAT8            = 701;             //double
//DESCR("numeric(precision, decimal), arbitrary precision number");
const int PG_DATA_TYPE_NUMERIC           = 1700;            //number

//---------------------------------------------------------
//DESCR("variable-length string, binary values escaped");
const int PG_DATA_TYPE_BYTEA             = 17;
//DESCR("single character");
const int PG_DATA_TYPE_CHAR              = 18;              //=BPCHAROID
//DESCR("63-byte type for storing system identifiers");
const int PG_DATA_TYPE_NAME              = 19;
//DESCR("variable-length string, no limit specified");
const int PG_DATA_TYPE_TEXT              = 25;
//DESCR("char(length), blank-padded string, fixed storage length");
const int PG_DATA_TYPE_BPCHAR            = 1042;
//DESCR("varchar(length), non-blank-padded string, variable storage length");
const int PG_DATA_TYPE_VARCHAR           = 1043;

//---------------------------------------------------------
//SELECT  NOW()::date,NOW()::time,NOW()::abstime,NOW()::timestamp,NOW()::timestamptz
//DESCR("date");
const int PG_DATA_TYPE_DATE              = 1082;            //2018-12-12
//DESCR("time of day");
const int PG_DATA_TYPE_TIME              = 1083;            //02:40:37.388487
//DESCR("absolute, limited-range date and time (Unix system time)");
const int PG_DATA_TYPE_ABSTIME           = 702;             //2018-12-12 02:40:37+00
//DESCR("date and time");
const int PG_DATA_TYPE_TIMESTAMP         = 1114;            //2018-12-12 02:40:37.388487
//DESCR("date and time with time zone");
const int PG_DATA_TYPE_TIMESTAMPTZ       = 1184;            //2018-12-12 02:40:37.388487+00

//---------------------------------------------------------
//尝试根据pgsql中的参数类型名称转换为对应的类型oid
inline int pg_data_type_by_name(const char* name)
{
    if (!name)
        return PG_DATA_TYPE_UNKNOW;

    switch ((*(uint32_t*)name))
    {
        case 0x32746e69:return PG_DATA_TYPE_INT2;           //int2
        case 0x00746e69:                         
        case 0x34746e69:return PG_DATA_TYPE_INT4;           //int,int4
        case 0x38746e69:return PG_DATA_TYPE_INT8;           //int8
        case 0x616f6c66:return name[5] == '4' ? PG_DATA_TYPE_FLOAT4 : PG_DATA_TYPE_FLOAT8;//float4,float8,float
        case 0x656d756e:return PG_DATA_TYPE_NUMERIC;        //numeric
        case 0x74786574:return PG_DATA_TYPE_TEXT;           //text
        case 0x63726176:return PG_DATA_TYPE_VARCHAR;        //varchar
        case 0x65746164:return PG_DATA_TYPE_DATE;           //date
        case 0x656d6974:
            if (name[4] == 0) 
                        return PG_DATA_TYPE_TIME;           //time
            else if (name[9] == 0) 
                        return PG_DATA_TYPE_TIMESTAMP;      //timestamp
            else 
                        return PG_DATA_TYPE_TIMESTAMPTZ;    //timestamptz
        default:        return PG_DATA_TYPE_UNKNOW;
    }
}

//---------------------------------------------------------
//尝试根据dbc数据类型转换为pg数据类型
inline int pg_data_type_by_dbc(rx_dbc::data_type_t t)
{
    switch (t)
    {
        case rx_dbc::DT_INT  :return PG_DATA_TYPE_INT4;
        case rx_dbc::DT_UINT :return PG_DATA_TYPE_INT8;
        case rx_dbc::DT_LONG :return PG_DATA_TYPE_INT8;
        case rx_dbc::DT_FLOAT:return PG_DATA_TYPE_FLOAT8;
        case rx_dbc::DT_DATE :return PG_DATA_TYPE_TIMESTAMP;
        case rx_dbc::DT_TEXT: return PG_DATA_TYPE_TEXT;
        default:      return PG_DATA_TYPE_TEXT;
    }
}

#endif