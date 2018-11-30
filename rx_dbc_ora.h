#ifndef	_RX_DBC_ORA_H_
#define	_RX_DBC_ORA_H_

#include "oci.h"                                            //����OCI�ӿ�,Ĭ����"3rd_deps\ora\include"

#include "rx_cc_macro.h"                                    //��������궨��
#include "rx_assert.h"                                      //�������
#include "rx_str_util_std.h"                                //��������ַ�������
#include "rx_str_util_ex.h"                                 //������չ�ַ�������
#include "rx_str_tiny.h"                                    //���붨���ַ�������
#include "rx_mem_alloc_cntr.h"                              //�����ڴ��������
#include "rx_datetime.h"                                    //��������ʱ�书��
#include "rx_dtl_buff.h"                                    //���뻺��������
#include "rx_dtl_array_ex.h"                                //�����������
#include "rx_ct_delegate.h"                                 //����ί�й���
#include "rx_datetime_ex.h"                                 //��������ʱ�书����չ
#include "rx_cc_atomic.h"                                   //����ԭ�ӱ�������

#include "dbc_comm/sql_param_parse.h"                       //SQL�󶨲��������ֽ�������

#include "dbc_ora/comm.h"                                   //ʵ��һЩͨ�ù���
#include "dbc_ora/conn.h"                                   //ʵ�����ݿ�����
#include "dbc_ora/param.h"                                  //ʵ�����ΰ󶨲���
#include "dbc_ora/stmt.h"                                   //ʵ��sql����
#include "dbc_ora/field.h"                                  //ʵ�ּ�¼�ֶβ�������
#include "dbc_ora/query.h"                                  //ʵ�ּ�¼��ѯ���ʶ���

//---------------------------------------------------------
namespace rx_dbc_ora
{
    //��־���������ί������
    typedef rx::delegate3_t<const char*,const char*, va_list,void> dbc_log_delegate_t;
    static inline void default_dbc_log_func(const char* type, const char* msg, va_list arg, void*) 
    { 
        static rx::atomic_t<uint32_t> msg_seq;

        char cur_time_str[20];
        rx_iso_datetime(cur_time_str);
        printf("[%s][%04d][%s]", cur_time_str, msg_seq.inc(),type);
        vprintf(msg, arg);
        puts("\n");
    }

    //-----------------------------------------------------
    //����Ӧ�ü�dbc���Ӷ���Ĺ��ܷ�װ,���з����������׳��쳣,����Ӧ�ò�ʹ��
    //-----------------------------------------------------
    class dbc_conn_t
    {
        conn_t          m_conn;
        conn_param_t    m_conn_param;
        env_option_t    m_env_param;
        friend class dbc_t;
        friend class tiny_dbc_t;
    public:
        //-------------------------------------------------
        dbc_log_delegate_t  log_func;                       //��־�������,Ĭ��Ϊdefault_dbc_log_func.
        dbc_conn_t() { log_func.bind(default_dbc_log_func); }
        dbc_conn_t(rx::mem_allotter_i& ma):m_conn(ma) { log_func.bind(default_dbc_log_func); }
        virtual ~dbc_conn_t() {}
        //-------------------------------------------------
        //��־������ܷ�װ
        void log_warn(const char* msg,...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("warn", msg, arg);
        }
        void log_err(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("err", msg, arg);
        }
        void log_info(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("info", msg, arg);
        }
        //-------------------------------------------------
        bool schema_to(const char *schema)
        {
            if (!connect())
                return false;

            try {
                m_conn.schema_to(schema);
                return true;
            }
            catch (error_info_t &e)
            {
                log_err(e.c_str(m_conn_param));
                return false;
            }
        }
        //-------------------------------------------------
        //�Ƿ�ʹ���������Ի���
        void use_chinese_env(bool flag = true) { flag ? m_env_param.use_chinese() : m_env_param.use_english(); }
        //-------------------------------------------------
        //�������Ӳ���
        void set_conn_param(const char* host,const char* user,const char* pwd,const char* db="oradb",uint16_t port=1521,uint16_t conn_timeout_sec=3) 
        {
            rx::st::strcpy(m_conn_param.host, sizeof(m_conn_param.host), host);
            rx::st::strcpy(m_conn_param.user, sizeof(m_conn_param.user), user);
            rx::st::strcpy(m_conn_param.pwd, sizeof(m_conn_param.pwd), pwd);
            rx::st::strcpy(m_conn_param.db, sizeof(m_conn_param.db), db);
            m_conn_param.port = port;
            m_conn_param.conn_timeout = conn_timeout_sec;
        }
        void set_conn_param(const conn_param_t &p) { set_conn_param(p.host,p.user,p.pwd,p.db,p.port,p.conn_timeout); }
        //-------------------------------------------------
        //�������Ӷ���,���������Ƿ�ɹ�
        //����ֵ:�����Ƿ�ɹ�
        bool connect(bool force_check=false, const char* schema = NULL)
        {
            if (force_check)
            {//���Ҫ��ǿ�Ƽ��,���������������ping����
                if (m_conn.ping())
                    return true;                            //���Ӽ��ɹ�,ֱ�ӷ���
            }
            else if (m_conn.is_valid())
                return true;

            //����,������Ч,��Ҫ�������Ӷ���
            try {
                m_conn.open(m_conn_param);
                if (!is_empty(schema))
                    m_conn.schema_to(schema);
                on_connect(m_conn);                         //����������ɶ����¼�
                return true;
            }
            catch (error_info_t &e)
            {
                log_err(e.c_str(m_conn_param));
                return false;
            }
        }

        //-------------------------------------------------
    protected:
        //��������¼�
        virtual void on_connect(conn_t& conn) {}
    };

    //-----------------------------------------------------
    //�����ݿ���ʹ��ܽ�����������װ,������������������ͳһ�쳣��׽����
    //-----------------------------------------------------
    class tiny_dbc_t
    {
        query_t                     m_query;                //ʵ�����ĵײ�ִ����
        dbc_conn_t                 &m_dbconn;               //���������ܶ��������

        //-------------------------------------------------
        //����ִ�����
        //����ֵ:<0����; 0�û�Ҫ�����; >0���
        int m_exec(const char* sql,bool manual_trans, void *usrdat)
        {
            try {
                if (manual_trans)
                    m_query.conn().trans_begin();           //�ص���������Ҫִ�ж������,��Ҫ�����ֶ�������

                int rc=on_exec(m_query, sql, usrdat);       //ִ���û����������,������ʼ
                if (rc <= 0)
                {
                    if (manual_trans)
                        m_query.conn().trans_rollback();    //Ҫ����������,�ع�
                    return rc;
                }

                if (m_query.sql_type()!=ST_SELECT)
                    m_query.conn().trans_commit();          //������ǲ�ѯ���,��������ύ

                return rc;
            }
            catch (error_info_t &e)
            {
                m_query.conn().trans_rollback();            //�����κδ���,�����Իع�
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));//����������־
                return -102;
            }
        }
    public:
        //-------------------------------------------------
        tiny_dbc_t(dbc_conn_t  &c):m_query(c.m_conn), m_dbconn(c){}
        //���¶���������ܷ���,�������׳��쳣�һ����������־,���ⲿ�����ߵĴ�����.
        //-------------------------------------------------
        //����sql�������ݰ󶨲�ִ��
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        int operator()(const char* sql, void *usrdat = NULL, bool manual_trans = false, bool can_retry = true)
        {
            if (!m_dbconn.connect())
                return -100;                                //�������ӻ����Ӽ��ʧ��,ֱ�ӷ���

            int rc = m_exec(sql,manual_trans, usrdat);      //����������ִ�ж���
            if (!can_retry || rc >= 0)
                return rc;                                  //�������Ի�ɹ����,ֱ�ӷ���
            
            //ִ��ʧ����Ҫ����
            if (!m_dbconn.connect(true))
                return -101;                                //ǿ�����ӻ����Ӽ��ʧ��,˵������ȷʵ�Ͽ���

            return m_exec(sql,manual_trans, usrdat);        //�����ٴ�ִ��
        }
    protected:
        //-------------------------------------------------
        //���ݰ�����¼�
        //-------------------------------------------------
        //ִ��ǰ�����¼�,���Խ��ж������Ĵ���
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_exec(query_t &q, const char *sql, void *usrdat) = 0;
    };


    //-----------------------------------------------------
    //�������ݰ��¼���ί������
    typedef rx::delegate1_t<query_t&, int32_t> dbc_delegate_t;
    //DBC�¼�ί�ж�Ӧ�ĺ���ָ������;//����ֵ:<0����; 0�û�Ҫ�����; >0���,�������
    typedef int32_t (*dbc_event_func_t)(query_t &q,void *usrdat);

    //-----------------------------------------------------
    //����Ӧ�ü�dbc������Ĺ��ܷ�װ,���з����������׳��쳣,����Ӧ�ò�����̳к�����ʵ�־���ҵ��
    //-----------------------------------------------------
    class dbc_t
    {
        query_t                     m_query;                //ʵ�����ĵײ�ִ����
        dbc_conn_t                 &m_dbconn;               //���������ܶ��������
        dbc_delegate_t              m_databind_dgt;         //���ݰ��¼���ί������
        dbc_delegate_t              m_datafetch_dgt;        //������ȡ�¼���ί������
        uint32_t                    m_bind_bulks;           //��¼ʵ��ִ��ʱ�󶨵������������
        //-------------------------------------------------
        //���в������ݵİ󶨴���
        //����ֵ:<0����; 0�û�Ҫ�����; >0���
        int m_bind_data(void *usrdat)
        {
            try {
                int bind_bulks = on_bind_data(m_query, usrdat);
                if (bind_bulks <= 0)
                    return bind_bulks;
                else
                    m_bind_bulks = bind_bulks;
                return m_bind_bulks;
            }
            catch (error_info_t &e)
            {
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));
                return -103;
            }
        }
        //-------------------------------------------------
        //����ִ�����
        //����ֵ:<0����; 0�û�Ҫ�����; >0���
        int m_exec(bool manual_trans, void *usrdat)
        {
            try {
                if (manual_trans)
                    m_query.conn().trans_begin();           //�ص���������Ҫִ�ж������,��Ҫ�����ֶ�������

                int rc=on_exec_befor(m_query, usrdat);      //ִ���û����������,������ʼ
                if (rc <= 0)
                {
                    if (manual_trans)
                        m_query.conn().trans_rollback();    //Ҫ����������,�ع�
                    return rc;
                }

                rc=on_exec_after(m_query, usrdat);          //ִ���û����������,��������
                if (rc <= 0)
                {
                    if (manual_trans)
                        m_query.conn().trans_rollback();    //Ҫ����������,�ع�
                    return rc;
                }

                if (m_query.sql_type()!=ST_SELECT)
                    m_query.conn().trans_commit();          //������ǲ�ѯ���,��������ύ

                return rc;
            }
            catch (error_info_t &e)
            {
                m_query.conn().trans_rollback();            //�����κδ���,�����Իع�
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));//����������־
                return -102;
            }
        }
    public:
        //-------------------------------------------------
        dbc_t(dbc_conn_t  &c):m_query(c.m_conn), m_dbconn(c){}
        //���¶���������ܷ���,�������׳��쳣�һ����������־,���ⲿ�����ߵĴ�����.
        //-------------------------------------------------
        //����sql����Ԥ����
        //����ֵ:�Ƿ�ɹ�
        bool prepare(const char* sql,...)
        {
            if (!m_dbconn.connect())                        //�������������Ӽ�鼴��,Ҫ��������Ч��
                return false;

            try {
                va_list arg;
                va_start(arg, sql);
                m_query.prepare(sql, arg);                  //Ԥ����,

                uint32_t max_bulk_deep = on_auto_bind();    //�ж��Ƿ�����Զ��󶨲���
                if (max_bulk_deep)
                    m_query.auto_bind(max_bulk_deep);       //�����Զ���
                else
                    on_bind_param(m_query);                 //���Խ����ֶ���
                return true;
            }
            catch (error_info_t &e)
            {
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));
                return false;
            }
        }
        //-------------------------------------------------
        //����sql�������ݰ󶨲�ִ��
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        int exec(void *usrdat=NULL,bool manual_trans=false,bool can_retry=true)
        {
            int rc = m_bind_data(usrdat);                   //����on_bind_data������
            if (rc <= 0)
                return rc;                                  //��������ݳ����Ҫ�����,��ô�Ͳ��ü�����

            if (!m_dbconn.connect())
                return -100;                                //�������ӻ����Ӽ��ʧ��,ֱ�ӷ���

            rc = m_exec(manual_trans, usrdat);              //����������ִ�ж���
            if (!can_retry || rc >= 0)
                return rc;                                  //�������Ի�ɹ����,ֱ�ӷ���
            
            //ִ��ʧ����Ҫ����
            if (!m_dbconn.connect(true))
                return -101;                                //ǿ�����ӻ����Ӽ��ʧ��,˵������ȷʵ�Ͽ���

            return m_exec(manual_trans, usrdat);            //�����ٴ�ִ��
        }
        //-------------------------------------------------
        //��ݷ���,ֱ�ӽ�����ִ��
        int operator()(const char* sql, void *usrdat = NULL, bool manual_trans = false, bool can_retry = true)
        {
            if (!prepare(sql))
                return -104;
            return exec(usrdat, manual_trans, can_retry);
        }
        //-------------------------------------------------
        //ִ����select��,���Խ��н������ȡ;�˷������Է�����ε���,ֱ������������
        //����ֵ:<0����;0����;>0������ȡ������
        int fetch(void *usrdat=NULL,uint32_t loop_count = 100)
        {
            if (m_query.sql_type() != ST_SELECT)
            {
                m_dbconn.log_err("non-select statements were fetched resultset! (%s)",m_query.sql_string());
                return -200;
            }

            try {
                uint32_t rc = 0;
                for (; !m_query.eof() && rc<loop_count; m_query.next(),++rc)
                    on_row_data(m_query, usrdat);
                return rc;
            }
            catch (error_info_t &e)
            {
                m_dbconn.log_err(e.c_str(m_dbconn.m_conn_param));
                return -201;
            }
        }
        //-------------------------------------------------
        //�������ݰ󶨴���ص�����(���funcΪ��,����Խ���usrdat�ĸ���)
        bool event_data_bind(dbc_event_func_t func, void *usrdat=NULL) 
        { 
            if (func == NULL)
                func = m_databind_dgt.cb_func();
            if (func == NULL)
                return false;
            m_databind_dgt.bind(func,usrdat); 
            return true;
        }
        //����������ȡ����Ļص�����
        bool event_data_row(dbc_event_func_t func, void *usrdat = NULL) 
        {
            if (func == NULL)
                func = m_datafetch_dgt.cb_func();
            if (func == NULL)
                return false;
            m_datafetch_dgt.bind(func, usrdat);
            return true;
        }

    protected:
        //�������е��¼�,�����ý��ж�����쳣��׽,ֻҪ��дҵ����뼴��
        //-------------------------------------------------
        //����������¼�
        //-------------------------------------------------
        //�Ƿ�����Զ�������
        //����ֵ:0���Զ��󶨲���;>0Ϊ�������ݿ������������
        virtual uint32_t on_auto_bind() { return 1; }
        //-------------------------------------------------
        //�����ֶ������󶨶���,��Ҫ��֪����������Ȳ���һ���в�����
        virtual void on_bind_param(query_t &q) { q.manual_bind(1); }
    protected:
        //-------------------------------------------------
        //���ݰ�����¼�
        //-------------------------------------------------
        //ִ��ǰ�����¼�,���Խ��ж������Ĵ���
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_exec_befor(query_t &q, void *usrdat) { q.exec(BAT_FETCH_SIZE, m_bind_bulks); return 1; }
        //-------------------------------------------------
        //ִ����ɺ�����¼�,���������on_bind_data���,����ʣ���������ƫ�����ĵ���
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_exec_after(query_t &q, void *usrdat) { return 1; }
        //-------------------------------------------------
        //!!�ؼ�!!���в������ݵİ󶨶���;
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_bind_data(query_t &q, void *usrdat)
        { 
            if (m_databind_dgt.is_valid()) 
                return m_databind_dgt(q);                   //Ĭ��ʵ��,����ʹ��ί�ж����еĺ������е���
            return 0; 
        }
    protected:
        //-------------------------------------------------
        //�����ȡ����¼�
        //-------------------------------------------------
        //!!�ؼ�!!��ȡ�����,���ʵ�ǰ������;
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_row_data(query_t &q,void *usrdat) { if (m_datafetch_dgt.is_valid()) m_datafetch_dgt(q); return 1; }
    };

    //-----------------------------------------------------
    //�﷨��,���ں���ָ������ݲ�ִ�����
    class dbc_exec_t
    {
        dbc_t   m_dbc;
    public:
        //��ʼ�󶨱�ҪԪ��
        dbc_exec_t(dbc_conn_t  &c, dbc_event_func_t func,void *usrdat=NULL):m_dbc(c) { m_dbc.event_data_bind(func,usrdat); }
        //ִ��sql���,��������ݺ��ٴ�ִ��
        int operator()(const char* sql, void *usrdat=NULL, dbc_event_func_t func=NULL)
        {
            if (!is_empty(sql)&&!m_dbc.prepare(sql))
                return -300;
            if (usrdat&&!m_dbc.event_data_bind(func, usrdat))
                return -301;
            return m_dbc.exec();
        }
        //�������ݺ��ٴ�ִ��
        int operator()(void *usrdat, dbc_event_func_t func = NULL)
        {
            if (!m_dbc.event_data_bind(func,usrdat))
                return -301;
            return m_dbc.exec();
        }
    };
}

#endif