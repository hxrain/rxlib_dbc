#ifndef	_RX_DBC_MYSQL_H_
#define	_RX_DBC_MYSQL_H_

#include "mysql.h"                                          //����mysql�ӿ�,Ĭ����"3rd_deps\mysql\include"
#include "errmsg.h"
#include "mysqld_error.h"

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

#include "dbc_mysql/comm.h"                                 //ʵ��һЩͨ�ù���
#include "dbc_mysql/conn.h"                                 //ʵ�����ݿ�����

/*

#include "dbc_mysql/param.h"                                //ʵ�����ΰ󶨲���
#include "dbc_mysql/stmt.h"                                 //ʵ��sql����
#include "dbc_mysql/field.h"                                //ʵ�ּ�¼�ֶβ�������
#include "dbc_mysql/query.h"                                //ʵ�ּ�¼��ѯ���ʶ���

//---------------------------------------------------------
namespace rx_dbc_mysql
{
    //OCI�����жϺ��Ĭ�ϴ��䳬ʱӦ������20sec����,���г�ʱ�ж�ʱ��Ҫע��

    //��־���������ί������
    typedef rx::delegate3_t<const char*,const char*, va_list,void> dbc_log_delegate_t;
    static inline void default_dbc_log_func(const char* type, const char* msg, va_list arg, void*)
    {
        static rx::atomic_t<uint32_t> msg_seq;

        char cur_time_str[20];
        rx_iso_datetime(cur_time_str);
        printf("[%s][%04d][%s]:", cur_time_str, msg_seq.inc(),type);
        vprintf(msg, arg);
        puts("");
    }

    //-----------------------------------------------------
    //����Ӧ�ü�dbc���Ӷ���Ĺ��ܷ�װ,���з����������׳��쳣,����Ӧ�ò�ʹ��
    //-----------------------------------------------------
    class dbc_conn_t
    {
        conn_t              m_conn;
        conn_param_t        m_conn_param;
        env_option_t        m_env_param;
        dbc_error_code_t    m_last_error;
        friend class dbc_t;
        friend class tiny_dbc_t;
    public:
        //-------------------------------------------------
        dbc_log_delegate_t  log_func;                       //��־�������,Ĭ��Ϊdefault_dbc_log_func.
        dbc_conn_t() { log_func.bind(default_dbc_log_func); }
        dbc_conn_t(rx::mem_allotter_i& ma):m_conn(ma) { log_func.bind(default_dbc_log_func); }
        dbc_error_code_t last_err() { return m_last_error; }
        virtual ~dbc_conn_t() {}
        //-------------------------------------------------
        //��־������ܷ�װ
        void log_warn(const char* msg,...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("warn", msg, arg);
            va_end(arg);
        }
        void log_err(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("errr", msg, arg);
            va_end(arg);
        }
        void log_info(const char* msg, ...)
        {
            va_list arg;
            va_start(arg, msg);
            log_func("info", msg, arg);
            va_end(arg);
        }
        //-------------------------------------------------
        //�л���ָ�����û���
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
                do_error(e, NULL);
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
        //����ֵ:�����Ƿ�ɹ�,0-����ʧ��;1��������;2���ӽ���;3�������.
        uint32_t connect(bool force_check=false)
        {
            if (force_check)
            {//���Ҫ��ǿ�Ƽ��,���������������ping����
                if (m_conn.ping())
                    return 1;                               //���Ӽ��ɹ�,ֱ�ӷ���
            }
            else if (m_conn.is_valid())
                return 1;

            set_last_error(DBEC_OK);

            //����,������Ч,��Ҫ�������ӻ���������
            try {
                bool is_opened = m_conn.is_valid();
                int16_t rc=m_conn.open(m_conn_param);
                if (rc)
                {
                    log_warn("connect with error code[%d]:host(%s),port(%d),user(%s),db(%s)", rc, m_conn_param.host, m_conn_param.port, m_conn_param.user, m_conn_param.db);
                    set_last_error(DBEC_OCI_PWD_WILLEXPIRE);
                }
                    
                on_connect(m_conn, m_conn_param);           //����������ɶ����¼�
                return is_opened?3:2;
            }
            catch (error_info_t &e)
            {
                do_error(e,NULL);
                return 0;
            }
        }

        //-------------------------------------------------
    protected:
        //-------------------------------------------------
        //�����ļ�¼����dbc�����
        void set_last_error(dbc_error_code_t e) { m_last_error = e; }
        //-------------------------------------------------
        //���д����¼����־���
        void do_error(error_info_t &e, query_t *q)
        {
            log_err(e.c_str(m_conn_param));                 //������쳣����
            set_last_error((dbc_error_code_t)e.dbc_error_code());//��¼����ͳһ������
            if (!q||!q->params()) return;                   //û����䴦������,��û�а󶨵Ĳ���,����

            rx::tiny_string_t<char, 1024> str;              //����ֲ�С������,׼��ƴװ������ֵ
            try {
                uint32_t bulks = q->bulks(false);
                uint32_t params = q->params();
                for (uint32_t bi = 0; bi < bulks; ++bi)
                {//������������Ƚ��б���
                    q->bulk(bi);                            //���ÿ����
                    str.assign();                           //��������λ

                    for (uint32_t i = 0; i < params; ++i)   //ѭ��ƴװ��ǰ����ȵĲ���ֵ
                    {
                        str << q->param(i).name() << '=' << q->param(i).as_string();
                        if (i + 1 < params) str << ' ';
                    }
                        
                    log_info("bulk[%d/%d]params(%d)<%s>", bi+1, bulks, q->params(),str.c_str());
                }
            }
            catch (...) {}
        }
        //-------------------------------------------------
        //��������¼�
        virtual void on_connect(conn_t& conn, const conn_param_t &param) {}
    };

    //-----------------------------------------------------
    //�����ݿ���ʹ��ܽ�����������װ,������������������ͳһ�쳣��׽����
    //-----------------------------------------------------
    class tiny_dbc_t
    {
        query_t                     m_query;                //ʵ�����ĵײ�ִ����
        dbc_conn_t                 &m_dbconn;               //���������ܶ��������
        friend class dbc_t;
        //-------------------------------------------------
        //Ԥ�������
        //����ֵ:<0����; 0�û�Ҫ�����; >0���
        int m_prepare(const char* sql)
        {
            try {
                m_query.prepare(sql);                       //Ԥ��������
                return 1;
            }
            catch (error_info_t &e)
            {
                m_dbconn.do_error(e, &m_query);
                return -103;
            }
        }
        //-------------------------------------------------
        //����ִ�����
        //����ֵ:<0����; 0�û�Ҫ�����; >0���
        int m_exec(bool explicit_trans, void *usrdat)
        {
            try {
                if (explicit_trans)
                    m_query.conn().trans_begin();           //�ص���������Ҫִ�ж������,��Ҫ�����ֶ�������

                int rc=on_exec(m_query,usrdat);             //ִ���û����������
                if (rc <= 0)
                {
                    if (explicit_trans)
                        m_query.conn().trans_rollback();    //Ҫ����������,�ع�
                    return rc;
                }

                if (m_query.sql_type()!=ST_SELECT)
                    m_query.conn().trans_commit();          //������ǲ�ѯ���,��������ύ

                return rc;
            }
            catch (error_info_t &e)
            {
                m_dbconn.do_error(e, &m_query);
                m_query.conn().trans_rollback();            //�����κδ���,�����Իع�
                return -102;
            }
        }
        //-------------------------------------------------
        //���������������Ķ�������
        int do_action(const char* sql, void *usrdat, bool explicit_trans, uint32_t retry)
        {
            if (!m_dbconn.connect())
                return -100;                                //�������ӻ����Ӽ��ʧ��,ֱ�ӷ���
            
            for (uint32_t ri = 0; ri <= retry; ++ri)
            {
                int rc;
                if (!is_empty(sql))
                {
                    rc = m_prepare(sql);                    //���������sql��Ԥ����
                    if (rc <= 0)
                        return rc;
                }

                rc = m_exec(explicit_trans, usrdat);        //����������ִ�ж���
                if (!retry || rc >= 0)
                    return rc;                              //�������Ի�ɹ����,ֱ�ӷ���

                uint32_t cc = m_dbconn.connect(true);       //ִ��ʧ����Ҫ����
                if (!cc)
                    return -101;                            //ǿ�����ӻ����Ӽ��ʧ��,˵������ȷʵ�Ͽ���

                if (cc != 3)
                    return rc;                              //������Ӷ������������������,���ü���������.
            }
            return -104;
        }
    public:
        //-------------------------------------------------
        tiny_dbc_t(dbc_conn_t  &c):m_query(c.m_conn), m_dbconn(c){}
        virtual ~tiny_dbc_t() {}
        //���¶���������ܷ���,�������׳��쳣�һ����������־,���ⲿ�����ߵĴ�����.
        //-------------------------------------------------
        //ִ��sql���,����������������;������δ������ݵ�ʱ����ָ��sql���(��NULL��"").
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        int action(const char* sql, void *usrdat = NULL, bool explicit_trans = false, uint32_t retry = 1)
        {
            const char* event_sql = is_empty(sql) ? m_query.sql_string() : sql;
            if (!on_begin(m_dbconn, m_query, event_sql, usrdat))//����������ʼǰ�Ĵ�������
                return 0;
            int rc = do_action(sql, usrdat, explicit_trans, retry);
            on_end(rc, m_dbconn, m_query, event_sql, usrdat);
            return rc;
        }
        int operator()(const char* sql, void *usrdat = NULL, bool explicit_trans = false, uint32_t retry = 1)
        {
            return action(sql,usrdat,explicit_trans,retry);
        }
        //-------------------------------------------------
        //ִ����select��,���Խ��н������ȡ;�˷������Է�����ε���,ֱ������������
        //����ֵ:<0����;0����;>0������ȡ������
        int fetch(uint32_t loop_count = 100)
        {
            m_dbconn.set_last_error(DBEC_OK);
            if (m_query.sql_type() != ST_SELECT)
            {
                m_dbconn.log_err("non-select statements were fetched resultset! (%s)", m_query.sql_string());
                m_dbconn.set_last_error(DBEC_METHOD_CALL);
                return -200;
            }

            try {
                uint32_t rc = 0;
                for (; !m_query.eof() && rc<loop_count; m_query.next(), ++rc)
                    on_row(m_query,&rc);
                return rc;
            }
            catch (error_info_t &e)
            {
                m_dbconn.do_error(e, &m_query);
                return -201;
            }
        }
    protected:
        //-------------------------------------------------
        //�������ʵ�ֶ���ǰ�󴥷��¼��Ļص�
        //������ʼ֮ǰ���д���(�ɽ��м�ʱ/����/���Եȴ���);����ֵ:�Ƿ����ִ��
        virtual bool on_begin(dbc_conn_t &conn, query_t &q, const char* sql, void *usrdat) { return true; }
        //�������֮����д���(�ɽ��м�ʱ,�����ȴ���)
        virtual void on_end(int rc,dbc_conn_t &conn, query_t &q, const char* sql, void *usrdat) {}
        //-------------------------------------------------
        //��ȡ�����,���ʵ�ǰ������;
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_row(query_t &q, void *usrdat) { return 1; }
        //-------------------------------------------------
        //���û��ṩִ���¼�,���Խ��ж������Ĵ���;sql�Ѿ���Ԥ������.
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_exec(query_t &q, void *usrdat) { q.exec(); return 1; }
    };


    //-----------------------------------------------------
    //����dbc�¼���ί������
    typedef rx::delegate1_t<query_t&, int32_t> dbc_delegate_t;
    //DBC�¼�ί�ж�Ӧ�ĺ���ָ������;//����ֵ:<0����; 0�û�Ҫ�����; >0���,�������
    typedef int32_t (*dbc_event_func_t)(query_t &q,void *usrdat);

    //-----------------------------------------------------
    //����Ӧ�ü�dbc������Ĺ��ܷ�װ,���з����������׳��쳣,����Ӧ�ò�����̳к�����ʵ�־���ҵ��
    //-----------------------------------------------------
    class dbc_t:public tiny_dbc_t
    {
        dbc_delegate_t              m_databind_dgt;         //���ݰ��¼���ί������
        dbc_delegate_t              m_datafetch_dgt;        //������ȡ�¼���ί������
    public:
        //-------------------------------------------------
        //���캯��,��db������ҵ�����ص�����
        dbc_t(dbc_conn_t  &c, dbc_event_func_t on_bind=NULL, dbc_event_func_t on_row=NULL):tiny_dbc_t(c)
        {
            if (on_bind) event_on_bind(on_bind);
            if (on_row) event_on_row(on_row);
        }
        virtual ~dbc_t() {}
        //-------------------------------------------------
        //ִ�������ṩ��sql���,��������
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        int action(void *usrdat, bool explicit_trans = false, uint32_t retry = 1)
        {
            if (is_empty(m_query.sql_string()))
                return tiny_dbc_t::action(on_sql(), usrdat, explicit_trans, retry);  //�״�ִ��,��Ҫ����sql
            else
                return tiny_dbc_t::action(NULL, usrdat, explicit_trans, retry);     //����ִ��,����Ҫ�ٽ���sql
        }
        //-------------------------------------------------
        //�������ݰ󶨴����ص�����
        void event_on_bind(dbc_event_func_t func) { m_databind_dgt.bind(func); }
        //����������ȡ�����Ļص�����
        void event_on_row(dbc_event_func_t func) { m_datafetch_dgt.bind(func); }
    protected:
        //-------------------------------------------------
        //ִ���¼�,���Խ��ж������Ĵ���
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_exec(query_t &q, void *usrdat)
        { 
            if (!q.params())
                q.auto_bind(BAT_BULKS_SIZE);                //�����Զ���,��֪����������
            int rc= on_bind_data(q, usrdat);                //����on_bind_data�¼�
            if (rc <= 0) return rc;
            q.exec(rc);                                     //ִ��������OCI/ORA����
            return 1; 
        }
        //-------------------------------------------------
        //!!�ؼ�!!��ȡ�����,���ʵ�ǰ������;
        //����ֵ:<0����;0�û�Ҫ�����;>0���
        virtual int32_t on_row(query_t &q, void *usrdat)
        {
            if (m_datafetch_dgt.is_valid())
            {
                m_datafetch_dgt.bind(m_datafetch_dgt.cb_func(), usrdat);
                return m_datafetch_dgt(q);                  //Ĭ��ʵ��,����ʹ��ί�ж����еĺ������е���
            }
            return 1;
        }
    protected:
        //-------------------------------------------------
        //ͨ���������sql���
        virtual const char* on_sql() { return NULL; }
        //-------------------------------------------------
        //!!�ؼ�!!���в������ݵİ󶨶���;
        //����ֵ:<0����;0�û�Ҫ�����;>0Ϊ������������
        virtual int32_t on_bind_data(query_t &q, void *usrdat)
        {
            if (m_databind_dgt.is_valid())
            {//ÿ�ζ���Ҫ���Ը����û������ݺ�����Ӧ������ָ��
                m_databind_dgt.bind(m_databind_dgt.cb_func(), usrdat);
                return m_databind_dgt(q);                   //����ί�ж����еĺ���
            }
            return 0;
        }
    };
}
*/

#endif