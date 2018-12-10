#ifndef _RX_DBC_TYPE_H_
#define _RX_DBC_TYPE_H_

namespace rx_dbc
{
    //����ʹ�õ��ַ�������󳤶�
    const unsigned short MAX_TEXT_BYTES = 1024 * 2;

    //ÿ������FEATCH��ȡ�Ľ����������
    const unsigned short BAT_FETCH_SIZE = 20;

    //sql���ĳ�������
    const int MAX_SQL_LENGTH = 1024 * 4;

    typedef const char* PStr;
    const int CHAR_SIZE = sizeof(char);

    //-----------------------------------------------------
    //DB����
    typedef enum db_type_t
    {
        DBT_UNKNOWN=-1,
        DBT_ORA,
        DBT_MYSQL,
        DBT_PGSQL,
        DBT_SQLITE,
    }db_type_t;

    //-----------------------------------------------------
    //DB���Ӳ���
    typedef struct conn_param_t
    {
        char        host[64];                               //���ݿ���������ڵ�ַ
        char        user[64];                               //���ݿ��û���
        char        pwd[64];                                //���ݿ����
        char        db[64];                                 //���ݿ�ʵ����
        int         port;                                   //���ݿ�˿�
        int         conn_timeout;                           //���ӳ�ʱʱ��
        conn_param_t()
        {
            host[0] = 0;
            db[0] = 0;
            user[0] = 0;
            pwd[0] = 0;
            port = 1521;
            conn_timeout = 3;
        }
    }conn_param_t;

    //-----------------------------------------------------
    //dbc_ora���Դ������������(�󶨲���ʱ,����ǰ׺���Ը�֪��������)
    typedef enum data_type_t
    {
        DT_UNKNOWN=-1,
        DT_LONG     = 'i',                                  //����������,long/int32_t
        DT_ULONG    = 'u',                                  //�޷�������,ulong/uint32_t
        DT_FLOAT    = 'f',                                  //������,double
        DT_DATE     = 'd',                                  //��������
        DT_TEXT     = 's'                                   //�ı�������
    }data_type_t;

    //-----------------------------------------------------
    //sql�������
    typedef enum sql_type_t
    {
        ST_UNKNOWN=-1,
        ST_SELECT   ,
        ST_UPDATE   ,
        ST_DELETE   ,
        ST_INSERT   ,
        ST_CREATE   ,
        ST_DROP     ,
        ST_ALTER    ,
        ST_BEGIN    ,
        ST_DECLARE  ,
        ST_SET
    }sql_type_t;
    
    //-----------------------------------------------------
    //DBC��װ����������
    typedef enum err_type_t
    {
        DBEC_OK = 0,
        DBEC_ENV_FAIL = 1000,                               //������������
        DBEC_NO_MEMORY,                                     //�ڴ治��
        DBEC_NO_BUFFER,                                     //����������
        DBEC_IDX_OVERSTEP,                                  //�±�Խ��
        DBEC_BAD_PARAM,                                     //��������
        DBEC_BAD_INPUT,                                     //���󶨲������������ʹ���
        DBEC_BAD_OUTPUT,                                    //��֧�ֵ������������
        DBEC_BAD_TYPEPREFIX,                                //�����Զ���ʱ,����ǰ׺��׼ȷ
        DBEC_UNSUP_TYPE,                                    //δ֧�ֵ���������
        DBEC_PARAM_NOT_FOUND,                               //���ʵĲ������󲻴���
        DBEC_FIELD_NOT_FOUND,                               //���ʵ��ж��󲻴���
        DBEC_METHOD_CALL,                                   //�������õ�˳�����
        DBEC_NOT_PARAM,                                     //sql�����û��':'ǰ׺�Ĳ���,�����԰󶨲���
        DBEC_PARSE_PARAM,                                   //sql����Զ�������������

        DBEC_DB,                                            //DB����
        DBEC_DB_BADPWD,                                     //DB����ϸ��:�˺ſ������
        DBEC_DB_PWD_WILLEXPIRE,                             //DB����ϸ��:���������,������������Ӧ�ý��и澯
        DBEC_DB_CONNTIMEOUT,                                //DB����ϸ��:���ӳ�ʱ
        DBEC_DB_CONNLOST,                                   //DB����ϸ��:�Ѿ����������ӶϿ���.
        DBEC_DB_CONNFAIL,                                   //DB����ϸ��:����ʧ��,�޷���������
        DBEC_DB_UNIQUECONST,                                //DB����ϸ��:ΨһԼ�����µĴ���

        DBEC_OTHER=-1,
    }err_type_t;


    inline const char* err_type_str(err_type_t dbc_err)
    {
        switch (dbc_err)
        {
        case	DBEC_ENV_FAIL:          return "(DBEC_ENV_FAIL):environment handle creation failed";
        case	DBEC_NO_MEMORY:         return "(DBEC_NO_MEMORY):memory allocation request has failed";
        case	DBEC_NO_BUFFER:         return "(DBEC_NO_BUFFER):memory buffer not enough";
        case	DBEC_IDX_OVERSTEP:      return "(DBEC_IDX_OVERSTEP):index access overstep the boundary";
        case	DBEC_BAD_PARAM:         return "(DBEC_BAD_PARAM):func param is incorrect";
        case	DBEC_BAD_INPUT:         return "(DBEC_BAD_INPUT):input bind data doesn't have expected type";
        case	DBEC_BAD_OUTPUT:        return "(DBEC_BAD_OUTPUT):output convert type incorrect";
        case	DBEC_BAD_TYPEPREFIX:    return "(DBEC_BAD_TYPEPREFIX):input bind parameter prefix incorrect";
        case	DBEC_UNSUP_TYPE:        return "(DBEC_UNSUP_TYPE):unsupported Oracle type - cannot be converted";
        case	DBEC_PARAM_NOT_FOUND:   return "(DBEC_PARAM_NOT_FOUND):name not found in statement's parameters";
        case	DBEC_FIELD_NOT_FOUND:   return "(DBEC_FIELD_NOT_FOUND):resultset doesn't contain field_t with such name";
        case    DBEC_METHOD_CALL:       return "(DBEC_METHOD_CALL):func method called order error";
        case    DBEC_NOT_PARAM:         return "(DBEC_NOT_PARAM):sql not parmas";
        case    DBEC_PARSE_PARAM:       return "(DBEC_PARSE_PARAM): auto bind sql param error";
        case    DBEC_DB:                return "(DBEC_DB_ERROR)";
        case    DBEC_DB_BADPWD:         return "(DBEC_DB_BADPWD)";
        case    DBEC_DB_PWD_WILLEXPIRE: return "(DBEC_DB_PWD_WILLEXPIRE)";
        case    DBEC_DB_CONNTIMEOUT:    return "(DBEC_DB_CONNTIMEOUT)";
        case    DBEC_DB_CONNLOST:       return "(DBEC_DB_CONNLOST)";
        case    DBEC_DB_CONNFAIL:       return "(DBEC_DB_CONNFAIL)";
        case    DBEC_DB_UNIQUECONST:    return "(DBEC_DB_UNIQUECONST)";
        default:                        return "(DBEC_UNKNOW_ERROR)";
        }
    }

}


#endif