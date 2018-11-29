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
    class dbc_conn_t
    {
        conn_t          m_conn;
        conn_param_t    m_conn_param;
        env_option_t    m_env_param;
        friend class dbc_stmt_t;
        friend class dbc_query_t;
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
    //����Ӧ�ü�dbc������Ĺ��ܷ�װ,���з����������׳��쳣,����Ӧ�ò�����̳к�����ʵ�־���ҵ��
    class dbc_stmt_t
    {
        stmt_t      m_stmt;
        dbc_conn_t  &m_dbc;
        //-------------------------------------------------
        bool m_exec()
        {
            try {

            }
            catch (error_info_t &e)
            {

            }
        }
    public:
        //-------------------------------------------------
        dbc_stmt_t(dbc_conn_t  &dbc):m_stmt(dbc.m_conn),m_dbc(dbc){}
        //-------------------------------------------------
        bool exec()
        {

        }
    };

    //-----------------------------------------------------
    //����Ӧ�ü�dbc��ѯ����Ĺ��ܷ�װ,���з����������׳��쳣,����Ӧ�ò�����̳к�����ʵ�־���ҵ��
    class dbc_query_t
    {
        query_t     m_query;
        dbc_conn_t  &m_dbc;
    public:
        dbc_query_t(dbc_conn_t  &dbc) :m_query(dbc.m_conn), m_dbc(dbc) {}
    };

}

#endif


