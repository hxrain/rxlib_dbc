#ifndef	_RX_DBC_PGSQL_FIELD_H_
#define	_RX_DBC_PGSQL_FIELD_H_

namespace pgsql
{
    /*
    begin;
    DECLARE cur CURSOR for select * from tmp_dbc;
    fetch 20 cur ;
    end
    */
    class query_t;
    //-----------------------------------------------------
    //�����ʹ�õ��ֶη��ʶ���
    //-----------------------------------------------------
    //�ֶ���󶨲���ʹ�õĻ���,�������ݻ������Ĺ������ֶ����ݵķ���
    class field_t:public col_base_t
    {
    public:
        field_t(){ }
        virtual ~field_t() { }
    };
}

#endif
