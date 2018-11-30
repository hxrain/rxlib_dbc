
#ifndef	_RX_DBC_ORA_CONN_H_
#define	_RX_DBC_ORA_CONN_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //����ORACLE�����ӵĹ��ܶ���
    class conn_t
    {
        //-------------------------------------------------
        //����������
        sword m_trans_alloc()
        {
            if (m_handle_trans)
            {
                rx_alert("conn_t������˳�����!");
                m_trans_free();
            }
            sword result=OCIHandleAlloc(m_handle_env, (void **)&m_handle_trans,  OCI_HTYPE_TRANS, 0, 0);
            if (result!=OCI_SUCCESS) return result;
            return OCIAttrSet(m_handle_svc, OCI_HTYPE_SVCCTX, m_handle_trans, 0, OCI_ATTR_TRANS, m_handle_err);
        }
        //-------------------------------------------------
        //�ͷ�������
        void m_trans_free()
        {
            if (!m_handle_trans) return;
            OCIAttrSet(m_handle_svc, OCI_HTYPE_SVCCTX, NULL, 0, OCI_ATTR_TRANS, m_handle_err);
            OCIHandleFree(m_handle_trans,OCI_HTYPE_TRANS);
            m_handle_trans=NULL;
        }
    public:
        //-------------------------------------------------
        conn_t(rx::mem_allotter_i& ma = rx_global_mem_allotter()):m_mem(ma)
        {
            m_handle_env = NULL;
            m_handle_err = NULL;
            m_handle_svc = NULL;

            m_handle_trans=NULL;
            m_handle_svr = NULL;
            m_handle_session = NULL;

            m_is_valid = false;
        }
        ~conn_t (){close();}
        //-------------------------------------------------
        bool is_valid(){return m_is_valid;}
        //-------------------------------------------------
        //���ӵ�Oracle������(���ֲ��ɴ�������ʱ���쳣,�ɿ�����ʱ����ora�������)
        //����ֵ:0����;���������˿ɿ�����(�������뼴�����ڵ���ʾ)
        sword open(const conn_param_t& dst, const env_option_t &op = env_option_t(), unsigned long env_mode = OCI_OBJECT | OCI_THREADED)
        {
            char dblink[1024];
            sprintf(dblink, "(DESCRIPTION=(CONNECT_TIMEOUT=%d)(TRANSPORT_CONNECT_TIMEOUT=%d)(ADDRESS=(PROTOCOL=tcp) (HOST=%s) (PORT=%d))(CONNECT_DATA=(SERVICE_NAME=%s)))",dst.conn_timeout,dst.conn_timeout,dst.host, dst.port, dst.db);
            return open(dblink,dst.user,dst.pwd,op,env_mode);
        }
        sword open (const char *dblink,const char *login,const char *password,const env_option_t &op = env_option_t(),unsigned long env_mode = OCI_OBJECT|OCI_THREADED)
        {
            if (is_empty(dblink) || is_empty(login) || is_empty(password))
                throw (error_info_t (DBEC_BAD_PARAM, __FILE__, __LINE__));
                
            close();
            //��ʼ��OCI����,�õ��������
            sword result = OCIEnvNlsCreate (&m_handle_env,env_mode,NULL,DBC_ORA_Malloc,DBC_ORA_Realloc,DBC_ORA_Free,0,NULL,op.charset_id, op.charset_id);
            if (result!=OCI_SUCCESS) throw (error_info_t (DBEC_ENV_FAIL, __FILE__, __LINE__));

            //����õ����������
            OCIHandleAlloc (m_handle_env,(void **) &m_handle_svr,OCI_HTYPE_SERVER,0,NULL);

            //����õ�������
            result = OCIHandleAlloc (m_handle_env,(void **) &m_handle_err,OCI_HTYPE_ERROR,0,NULL);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_env, __FILE__, __LINE__));

            //!!���ӵ�������!!
            result = OCIServerAttach (m_handle_svr,m_handle_err,(text *) dblink,(ub4)strlen (dblink),OCI_DEFAULT);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //����õ����ܾ��
            result = OCIHandleAlloc (m_handle_env,(void **) &m_handle_svc,OCI_HTYPE_SVCCTX,0,NULL);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //�󶨹��ܾ���������������
            result = OCIAttrSet (m_handle_svc,OCI_HTYPE_SVCCTX,m_handle_svr,sizeof (OCIServer *),OCI_ATTR_SERVER,m_handle_err);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //����õ��û��Ự���
            result = OCIHandleAlloc (m_handle_env,(void **) &m_handle_session,OCI_HTYPE_SESSION,0,NULL);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //���û��������û��Ự���
            result = OCIAttrSet (m_handle_session,OCI_HTYPE_SESSION,(text *) login,(ub4)strlen (login),OCI_ATTR_USERNAME,m_handle_err);
            if (result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            result = OCIAttrSet (m_handle_session,OCI_HTYPE_SESSION,(text *) password,(ub4)strlen (password),OCI_ATTR_PASSWORD,m_handle_err);
            if (result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            sword ec = 0;
            //!!���е�¼��֤!!
            result = OCISessionBegin(m_handle_svc,m_handle_err,m_handle_session,OCI_CRED_RDBMS,OCI_DEFAULT);
            if (result == OCI_SUCCESS_WITH_INFO)
            {
                char tmp[128];
                get_last_error(ec, tmp, sizeof(tmp));
                ec = ec;
            }
            else if(result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            //�󶨹��ܾ�������Ự���
            result = OCIAttrSet (m_handle_svc,OCI_HTYPE_SVCCTX,m_handle_session,sizeof (OCISession *),OCI_ATTR_SESSION,m_handle_err);
            if (result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            //�������ûỰ��������
            if (!is_empty(op.language))
                exec("ALTER SESSION SET NLS_LANGUAGE='%s'", op.language);

            //�������ûỰ�ı������ڸ�ʽ
            if (!is_empty(op.date_format))
                exec("ALTER SESSION SET NLS_DATE_FORMAT='%s'", op.date_format);

            m_is_valid = true;
            return ec;
        }
        //-------------------------------------------------
        //�رյ�ǰ������,�����׳��쳣
        bool close (void)
        {
            sword	result = OCI_SUCCESS;
            ub4 ec = 0;

            m_trans_free();                                 //�������ͷ�������

            if (m_handle_session != NULL)                   //!!�����û��Ự!!
                result = OCISessionEnd (m_handle_svc,m_handle_err,m_handle_session,OCI_DEFAULT);
            if (result != OCI_SUCCESS) ++ec;

            
            if (m_handle_svr)                               //!!�Ͽ�����������!!
                result = OCIServerDetach (m_handle_svr,m_handle_err,OCI_DEFAULT);
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_session != NULL)                   //�ͷŻỰ���
                result = OCIHandleFree(m_handle_session, OCI_HTYPE_SESSION);
            m_handle_session = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_svc != NULL)                       //�ͷŹ��ܾ��
                result = OCIHandleFree (m_handle_svc,OCI_HTYPE_SVCCTX);
            m_handle_svc = NULL;                         
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_err != NULL)                       //�ͷŴ�����
                result = OCIHandleFree (m_handle_err,OCI_HTYPE_ERROR);
            m_handle_err = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_svr != NULL)                       //�ͷŷ��������
                result = OCIHandleFree (m_handle_svr,OCI_HTYPE_SERVER);
            m_handle_svr = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_env != NULL)                       //�ͷŻ������
                result = OCIHandleFree (m_handle_env,OCI_HTYPE_ENV);
            m_handle_env = NULL;
            if (result != OCI_SUCCESS) ++ec;
                   
            m_is_valid = false;
            rx_assert(ec == 0);
            return ec == 0;
        }
        //-------------------------------------------------
        //ִ��һ��û�н������(��SELECT)��sql���
        //�ڲ�ʵ���ǽ�������ʱ��OiCommand����,����Ƶ��ִ�еĶ���������ʹ�ô˺���
        void exec(const char *sql,...);
        //-------------------------------------------------
        //�л���ָ�����û�ר����
        void schema_to(const char *schema) { exec("ALTER SESSION SET CURRENT_SCHEMA = %s", schema); }
        //-------------------------------------------------
        //��ǰ������������
        void trans_begin()
        {
            rx_assert(m_is_valid);
            sword result=m_trans_alloc();
            if (result!=OCI_SUCCESS)
                throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            result=OCITransStart(m_handle_svc,m_handle_err,0,OCI_TRANS_NEW);
            if (result != OCI_SUCCESS)
                throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //�ύ��ǰ����
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            sword result=OCITransCommit(m_handle_svc,m_handle_err,OCI_DEFAULT);
            if (result != OCI_SUCCESS)
                throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));
            m_trans_free();                                 //�ύ�ɹ�,�ͷ�������,����ع�ʱ�ͷ�
        }
        //-------------------------------------------------
        //�ع���ǰ����
        //����ֵ:�������(�ع��������ʱ�����׳��쳣)
        bool trans_rollback (sword *ec=NULL)
        {
            rx_assert(m_is_valid);
            sword result=OCITransRollback(m_handle_svc,m_handle_err,OCI_DEFAULT);
            if (result!=OCI_SUCCESS&&ec)
            {//���Բ鿴����ԭ��,�Ͳ����쳣��
                char tmp[128];
                get_last_error(*ec, tmp, sizeof(tmp));
            }
            m_trans_free();                                 //�ͷ�������
            return result == OCI_SUCCESS;
        }
        //-------------------------------------------------
        //���з�����ping���,��ʵ���ж������Ƿ���Ч.�����׳��쳣
        bool ping()
        {
            if (m_handle_svc == NULL)
                return false;
            sword result = OCIPing(m_handle_svc, m_handle_err, OCI_DEFAULT);
            return result == OCI_SUCCESS;
        }
        //-------------------------------------------------
        //�õ�ָ��������ı��ػ�������Ϣ(���� OCI_NLS_LANGUAGE)
        //����ֵ:�������(����ʱ�����׳��쳣)
        bool nls_info(ub2 InfoItem,char* Buf,ub4 BufSize, sword *ec = NULL)
        {
            rx_assert(m_is_valid);
            sword result=OCINlsGetInfo(m_handle_session,m_handle_err,(oratext*)Buf,BufSize,InfoItem);
            if (result != OCI_SUCCESS&&ec)
            {//���Բ鿴����ԭ��,�Ͳ����쳣��
                char tmp[128];
                get_last_error(*ec, tmp, sizeof(tmp));
            }
            return result == OCI_SUCCESS;
        }
        //-------------------------------------------------
        //��ȡ����oci�����ec,���Ӧ�Ĵ�������
        //����ֵ:�������(����ʱ�����׳��쳣)
        bool get_last_error(sword &ec,char *buff,ub4 max_size)
        {
            if (m_handle_err)
                return OCI_SUCCESS == OCIErrorGet(m_handle_err, 1, NULL, &ec, reinterpret_cast<text *> (buff), max_size, OCI_HTYPE_ERROR);
            else if (m_handle_env)
                return OCI_SUCCESS == OCIErrorGet(m_handle_env, 1, NULL, &ec, reinterpret_cast<text *> (buff), max_size, OCI_HTYPE_ENV);
            return false;
        }
        //-------------------------------------------------
    private:
        friend class stmt_t;
        friend class sql_param_t;
        friend class query_t;
        friend class field_t;

        rx::mem_allotter_i &m_mem;                          //�ڴ������
        bool		        m_is_valid;

        OCIEnv		        *m_handle_env;                  //OCI�������
        OCIServer	        *m_handle_svr;                  //OCI���������
        mutable OCIError	*m_handle_err;	                //OCI������.�����������ᱻOCI�ڲ��ı�
        OCISession	        *m_handle_session;              //OCI�û��Ự���
        OCISvcCtx	        *m_handle_svc;                  //OCIҵ���ܾ��
        OCITrans            *m_handle_trans;                //OCI��ȷ�ֶ�������������

        conn_t (const conn_t&);
        conn_t& operator = (const conn_t&);
    };
}

#endif
