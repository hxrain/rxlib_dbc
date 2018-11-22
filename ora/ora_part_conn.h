
#ifndef	_RX_DBC_ORA_CONN_H_
#define	_RX_DBC_ORA_CONN_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //����ORACLE�����ӵĹ��ܶ���
    class conn_t
    {
        sword m_trans_alloc()
        {
            if (m_TransHandle)
            {
                rx_alert("conn_t�������Ĵ���˳��������!");
                return OCI_SUCCESS;
            }
            sword result=OCIHandleAlloc(m_EnvHandle, (void **)&m_TransHandle,  OCI_HTYPE_TRANS, 0, 0);
            if (result!=OCI_SUCCESS) return result;
            return OCIAttrSet(m_SvcHandle, OCI_HTYPE_SVCCTX, m_TransHandle, 0,OCI_ATTR_TRANS, m_ErrHandle);
        }
        void m_trans_free()
        {
            if (!m_TransHandle) return;
            OCIAttrSet(m_SvcHandle, OCI_HTYPE_SVCCTX, 0, 0,OCI_ATTR_TRANS, m_ErrHandle);
            OCIHandleFree(m_TransHandle,OCI_HTYPE_TRANS);
            m_TransHandle=NULL;
        }
    public:
        //-------------------------------------------------
        conn_t(rx::mem_allotter_i& ma = rx_global_mem_allotter()):m_MemPool(ma)
        {
            m_EnvHandle = NULL;
            m_ErrHandle = NULL;
            m_SvcHandle = NULL;

            m_TransHandle=NULL;
            m_ServerHandle = NULL;
            m_SessionHandle = NULL;

            m_IsOpened = false;
        }
        ~conn_t (){close();}
        //-------------------------------------------------
        bool is_valid(){return m_IsOpened;}
        //-------------------------------------------------
        //���ӵ�Oracle������
        void open(const conn_param_t& dst, const env_option_t &op = env_option_t(), unsigned long env_mode = OCI_OBJECT | OCI_THREADED)
        {
            char dst_str[1024];
            sprintf(dst_str, "(DESCRIPTION=(CONNECT_TIMEOUT=%d)(TRANSPORT_CONNECT_TIMEOUT=%d)(ADDRESS=(PROTOCOL=tcp) (HOST=%s) (PORT=%d))(CONNECT_DATA=(SERVICE_NAME=%s)))",dst.conn_timeout,dst.tran_timeout,dst.host, dst.port, dst.db);
            open(dst_str,dst.user,dst.pwd,op,env_mode);
        }
        //-------------------------------------------------
        void open (const char *service_name,const char *login,const char *password,const env_option_t &op = env_option_t(),unsigned long env_mode = OCI_OBJECT|OCI_THREADED)
        {
            if (is_empty(service_name) || is_empty(login) || is_empty(password))
                throw (rx_dbc_ora::error_info_t (EC_BAD_PARAM_TYPE, __FILE__, __LINE__));
                
            if (m_IsOpened) return;
            close();
            //��ʼ��OCI����,�õ��������
            sword result = OCIEnvNlsCreate (&m_EnvHandle,env_mode,NULL,DBG_Malloc_Func,DBG_Realloc_Func,DBG_Free_Func,0,NULL,op.charset_id, op.charset_id);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (EC_ENV_CREATE_FAILED, __FILE__, __LINE__));

            //����õ����������
            OCIHandleAlloc (m_EnvHandle,(void **) &m_ServerHandle,OCI_HTYPE_SERVER,0,NULL);

            //����õ�������
            result = OCIHandleAlloc (m_EnvHandle,(void **) &m_ErrHandle,OCI_HTYPE_ERROR,0,NULL);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_EnvHandle, __FILE__, __LINE__));

            //��������������ݿ�ʵ������,!!���ӵ�������!!
            result = OCIServerAttach (m_ServerHandle,m_ErrHandle,(text *) service_name,(ub4)strlen (service_name),OCI_DEFAULT);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            //����õ�������
            result = OCIHandleAlloc (m_EnvHandle,(void **) &m_SvcHandle,OCI_HTYPE_SVCCTX,0,NULL);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            //�󶨷����������������
            result = OCIAttrSet (m_SvcHandle,OCI_HTYPE_SVCCTX,m_ServerHandle,sizeof (OCIServer *),OCI_ATTR_SERVER,m_ErrHandle);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            //����õ��û��Ự���
            result = OCIHandleAlloc (m_EnvHandle,(void **) &m_SessionHandle,OCI_HTYPE_SESSION,0,NULL);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            //���û��������û��Ự���
            result = OCIAttrSet (m_SessionHandle,OCI_HTYPE_SESSION,(text *) login,(ub4)strlen (login),OCI_ATTR_USERNAME,m_ErrHandle);
            if (result != OCI_SUCCESS) throw (rx_dbc_ora::error_info_t(result, m_ErrHandle, __FILE__, __LINE__));

            result = OCIAttrSet (m_SessionHandle,OCI_HTYPE_SESSION,(text *) password,(ub4)strlen (password),OCI_ATTR_PASSWORD,m_ErrHandle);
            if (result != OCI_SUCCESS) throw (rx_dbc_ora::error_info_t(result, m_ErrHandle, __FILE__, __LINE__));

            //�ڷ�������������ǰ�Ự,!!���е�¼��֤!!
            result = OCISessionBegin(m_SvcHandle,m_ErrHandle,m_SessionHandle,OCI_CRED_RDBMS,OCI_DEFAULT);
            if (result != OCI_SUCCESS) throw (rx_dbc_ora::error_info_t(result, m_ErrHandle, __FILE__, __LINE__));

            //���û��Ự��������񻷾������
            result = OCIAttrSet (m_SvcHandle,OCI_HTYPE_SVCCTX,m_SessionHandle,sizeof (OCISession *),OCI_ATTR_SESSION,m_ErrHandle);
            if (result != OCI_SUCCESS) throw (rx_dbc_ora::error_info_t(result, m_ErrHandle, __FILE__, __LINE__));

            if (!is_empty(op.language))
            {
                char SQL[128];
                sprintf(SQL,"ALTER SESSION SET NLS_LANGUAGE='%s'", op.language);
                exec(SQL);
            }

            if (!is_empty(op.date_format))
            {
                char SQL[128];
                sprintf(SQL,"ALTER SESSION SET NLS_DATE_FORMAT='%s'", op.date_format);
                exec(SQL);
            }
            m_IsOpened = true;
        }
        //-------------------------------------------------
        //�رյ�ǰ������
        bool close (void)
        {
            sword	result = OCI_SUCCESS;
            ub4 ec = 0;

            m_trans_free();                                    //�������ͷ�������

            if (m_SessionHandle != NULL)                    //�����û��Ự
                result = OCISessionEnd (m_SvcHandle,m_ErrHandle,m_SessionHandle,OCI_DEFAULT);
                
            if (result != OCI_SUCCESS) ++ec;

            //���������ӷ���������ϰ���
            if (m_ServerHandle&&m_ErrHandle)
                result = OCIServerDetach (m_ServerHandle,m_ErrHandle,OCI_DEFAULT);
            if (result != OCI_SUCCESS) ++ec;

            if (m_SvcHandle != NULL)                        //�ͷŷ��񻷾����
                result = OCIHandleFree (m_SvcHandle,OCI_HTYPE_SVCCTX);
            m_SvcHandle = NULL;                         
            if (result != OCI_SUCCESS) ++ec;

            if (m_SessionHandle != NULL)                    //�ͷŻỰ���
                result = OCIHandleFree (m_SessionHandle,OCI_HTYPE_SESSION);
            m_SessionHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_ErrHandle != NULL)                        //�ͷŴ�����
                result = OCIHandleFree (m_ErrHandle,OCI_HTYPE_ERROR);
            m_ErrHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_ServerHandle != NULL)                     //�ͷŷ��������
                result = OCIHandleFree (m_ServerHandle,OCI_HTYPE_SERVER);
            m_ServerHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_EnvHandle != NULL)                        //�ͷŻ������
                result = OCIHandleFree (m_EnvHandle,OCI_HTYPE_ENV);
            m_EnvHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;
                   
            m_IsOpened = false;
            return ec == 0;
        }
        //-------------------------------------------------
        //ִ��һ��û�н������(��SELECT)��SQL���
        //�ڲ�ʵ���ǽ�������ʱ��OiCommand����,����Ƶ��ִ�еĶ���������ʹ�ô˺���
        void exec(const char *sql_block,int sql_len = -1);
        //-------------------------------------------------
        //��ȷ���ڵ�ǰ����������һ������
        void trans_begin()
        {
            rx_assert(m_IsOpened);
            sword result=m_trans_alloc();
            if (result!=OCI_SUCCESS)
                throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            result=OCITransStart(m_SvcHandle,m_ErrHandle,0,OCI_TRANS_NEW);

            if (result!=OCI_SUCCESS)
				m_trans_free();
        }
        //-------------------------------------------------
        //�ύ�ڱ�����������еĸı�,����ÿ���ı䶼��������Ĭ������,���Ըı������ύ
        bool trans_commit (void)
        {
            rx_assert(m_IsOpened);
            sword result=OCITransCommit(m_SvcHandle,m_ErrHandle,OCI_DEFAULT);
            m_trans_free();
			return result==OCI_SUCCESS;
        }
        //-------------------------------------------------
        //�ع��ڱ�����������еĸı�,Ϊ�˱���ķ���,�ع��������ʱ�����׳��쳣
        bool trans_rollback (void)
        {
            bool Ret;
            rx_assert(m_IsOpened);
            sword result=OCITransRollback(m_SvcHandle,m_ErrHandle,OCI_DEFAULT);
            if (result!=OCI_SUCCESS)
            {//���Բ鿴����ԭ��,�Ͳ����쳣��
                rx_dbc_ora::error_info_t Error(result, m_ErrHandle, __FILE__, __LINE__);
                rx_alert("rollback fail");
                Ret=false;
            }
            else
                Ret=true;
            m_trans_free();
            return Ret;
        }
        //-------------------------------------------------
        //�õ�ָ��������ı��ػ�������Ϣ(���� OCI_NLS_LANGUAGE)
        bool nls_info(ub2 InfoItem,char* Buf,ub4 BufSize)
        {
            sword result=OCINlsGetInfo(m_SessionHandle,m_ErrHandle,(oratext*)Buf,BufSize,InfoItem);
            if (result!=OCI_SUCCESS)
            {//���Բ鿴����ԭ��,�Ͳ����쳣��
                rx_dbc_ora::error_info_t Error(result, m_ErrHandle, __FILE__, __LINE__);
                rx_alert("OCINlsGetInfo fail");
                return false;
            }
            return true;
        }
        //-------------------------------------------------
    private:
        friend class stmt_t;
        friend class sql_param_t;
        friend class query_t;
        friend class field_t;

        rx::mem_allotter_i   &m_MemPool;                    //�ڴ������
        bool		        m_IsOpened;

        OCIEnv		        *m_EnvHandle;                   //OCI�������
        OCIServer	        *m_ServerHandle;                //OCI���������
        mutable OCIError	*m_ErrHandle;	                //OCI������.�����������ᱻOCI�ڲ��ı�
        OCISession	        *m_SessionHandle;               //OCI�û��Ự���
        OCISvcCtx	        *m_SvcHandle;                   //OCI���񻷾����
        OCITrans            *m_TransHandle;                 //OCI��ȷ�ֶ�������������

        conn_t (const conn_t&);
        conn_t& operator = (const conn_t&);
    };
}

#endif
