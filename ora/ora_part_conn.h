
#ifndef	_RX_DBC_ORA_CONN_H_
#define	_RX_DBC_ORA_CONN_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //管理ORACLE的连接的功能对象
    class conn_t
    {
        sword m_trans_alloc()
        {
            if (m_TransHandle)
            {
                rx_alert("conn_t事务开启的处理顺序有问题!");
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
        conn_t(rx::mem_allotter_i& ma):m_MemPool(ma)
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
        //连接到Oracle服务器
        void open (const char *service_name,const char *login,const char *password,unsigned long env_mode = OCI_OBJECT|OCI_THREADED,
            ub2 charsetId=OCI_ZHS16GBK_CHARSET_ID,ub2 ncharsetId=OCI_ZHS16GBK_CHARSET_ID,const char* LanguageName=OCI_LANGUAGE_CHINA,const char* DateFmt=OCI_DEF_DATE_FORMAT)
        {
            if (is_empty(service_name) || is_empty(login) || is_empty(password))
                throw (rx_dbc_ora::error_info_t (EC_BAD_PARAM_TYPE, __FILE__, __LINE__));
                
            if (m_IsOpened) return;
            close();
            //初始化OCI环境,得到环境句柄
            sword result = OCIEnvNlsCreate (&m_EnvHandle,env_mode,NULL,DBG_Malloc_Func,DBG_Realloc_Func,DBG_Free_Func,0,NULL,charsetId,ncharsetId);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (EC_ENV_CREATE_FAILED, __FILE__, __LINE__));

            //分配得到服务器句柄
            OCIHandleAlloc (m_EnvHandle,(void **) &m_ServerHandle,OCI_HTYPE_SERVER,0,NULL);

            //分配得到错误句柄
            result = OCIHandleAlloc (m_EnvHandle,(void **) &m_ErrHandle,OCI_HTYPE_ERROR,0,NULL);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_EnvHandle, __FILE__, __LINE__));

            //服务器句柄绑定数据库实例描述
            result = OCIServerAttach (m_ServerHandle,m_ErrHandle,(text *) service_name,(ub4)strlen (service_name),OCI_DEFAULT);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            //分配得到服务句柄
            result = OCIHandleAlloc (m_EnvHandle,(void **) &m_SvcHandle,OCI_HTYPE_SVCCTX,0,NULL);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_EnvHandle, __FILE__, __LINE__));

            //绑定服务器句柄到服务句柄
            result = OCIAttrSet (m_SvcHandle,OCI_HTYPE_SVCCTX,m_ServerHandle,sizeof (OCIServer *),OCI_ATTR_SERVER,m_ErrHandle);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_ErrHandle, __FILE__, __LINE__));

            //分配得到用户会话句柄
            result = OCIHandleAlloc (m_EnvHandle,(void **) &m_SessionHandle,OCI_HTYPE_SESSION,0,NULL);
            if (result!=OCI_SUCCESS) throw (rx_dbc_ora::error_info_t (result, m_EnvHandle, __FILE__, __LINE__));

            //绑定用户名与口令到用户会话句柄
            result = OCIAttrSet (m_SessionHandle,OCI_HTYPE_SESSION,(text *) login,(ub4)strlen (login),OCI_ATTR_USERNAME,m_ErrHandle);

            if (result == OCI_SUCCESS)
                result = OCIAttrSet (m_SessionHandle,OCI_HTYPE_SESSION,(text *) password,(ub4)strlen (password),OCI_ATTR_PASSWORD,m_ErrHandle);

            //在服务句柄上启动当前会话,进行登录连接
            if (result == OCI_SUCCESS)
                result = OCISessionBegin(m_SvcHandle,m_ErrHandle,m_SessionHandle,OCI_CRED_RDBMS,OCI_DEFAULT);

            //绑定用户会话句柄到服务环境句柄上
            if (result == OCI_SUCCESS)
                result = OCIAttrSet (m_SvcHandle,OCI_HTYPE_SVCCTX,m_SessionHandle,sizeof (OCISession *),OCI_ATTR_SESSION,m_ErrHandle);

            if (!is_empty(LanguageName))
            {
                char SQL[128];
                sprintf(SQL,"ALTER SESSION SET NLS_LANGUAGE='%s'",LanguageName);
                exec(SQL);
            }

            if (!is_empty(DateFmt))
            {
                char SQL[128];
                sprintf(SQL,"ALTER SESSION SET NLS_DATE_FORMAT='%s'",DateFmt);
                exec(SQL);
            }
            m_IsOpened = true;
        }
        //-------------------------------------------------
        //关闭当前的连接
        bool close (void)
        {
            sword	result = OCI_SUCCESS;
            ub4 ec = 0;

            m_trans_free();                                    //尝试先释放事务句柄

            if (m_SessionHandle != NULL)                    //结束用户会话
                result = OCISessionEnd (m_SvcHandle,m_ErrHandle,m_SessionHandle,OCI_DEFAULT);
                
            if (result != OCI_SUCCESS) ++ec;

            //将错误句柄从服务器句柄上剥离
            result = OCIServerDetach (m_ServerHandle,m_ErrHandle,OCI_DEFAULT);
            if (result != OCI_SUCCESS) ++ec;

            if (m_SvcHandle != NULL)                        //释放服务环境句柄
                result = OCIHandleFree (m_SvcHandle,OCI_HTYPE_SVCCTX);
            m_SvcHandle = NULL;                         
            if (result != OCI_SUCCESS) ++ec;

            if (m_SessionHandle != NULL)                    //释放会话句柄
                result = OCIHandleFree (m_SessionHandle,OCI_HTYPE_SESSION);
            m_SessionHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_ErrHandle != NULL)                        //释放错误句柄
                result = OCIHandleFree (m_ErrHandle,OCI_HTYPE_ERROR);
            m_ErrHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_ServerHandle != NULL)                     //释放服务器句柄
                result = OCIHandleFree (m_ServerHandle,OCI_HTYPE_SERVER);
            m_ServerHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_EnvHandle != NULL)                        //释放环境句柄
                result = OCIHandleFree (m_EnvHandle,OCI_HTYPE_ENV);
            m_EnvHandle = NULL;
            if (result != OCI_SUCCESS) ++ec;
                   
            m_IsOpened = false;
            return ec == 0;
        }
        //-------------------------------------------------
        //执行一条没有结果返回(非SELECT)的SQL语句
        //内部实现是建立了临时的OiCommand对象,对于频繁执行的动作不建议使用此函数
        void exec(const char *sql_block,int sql_len = -1);
        //-------------------------------------------------
        //明确的在当前连接上启动一个事务
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
        //提交在本连接上面进行的改变,由于每个改变都是启动的默认事务,所以改变后必须提交
        bool trans_commit (void)
        {
            rx_assert(m_IsOpened);
            sword result=OCITransCommit(m_SvcHandle,m_ErrHandle,OCI_DEFAULT);
            m_trans_free();
			return result==OCI_SUCCESS;
        }
        //-------------------------------------------------
        //回滚在本连接上面进行的改变,为了编码的方便,回滚本身出错时不再抛出异常
        bool trans_rollback (void)
        {
            bool Ret;
            rx_assert(m_IsOpened);
            sword result=OCITransRollback(m_SvcHandle,m_ErrHandle,OCI_DEFAULT);
            if (result!=OCI_SUCCESS)
            {//可以查看错误原因,就不抛异常了
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
        //得到指定内容项的本地化配置信息(比如 OCI_NLS_LANGUAGE)
        bool nls_info(ub2 InfoItem,char* Buf,ub4 BufSize)
        {
            sword result=OCINlsGetInfo(m_SessionHandle,m_ErrHandle,(oratext*)Buf,BufSize,InfoItem);
            if (result!=OCI_SUCCESS)
            {//可以查看错误原因,就不抛异常了
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

        rx::mem_allotter_i   &m_MemPool;                    //内存分配器
        bool		        m_IsOpened;

        OCIEnv		        *m_EnvHandle;                   //OCI环境句柄
        OCIServer	        *m_ServerHandle;                //OCI服务器句柄
        mutable OCIError	*m_ErrHandle;	                //OCI错误句柄.可能这个句柄会被OCI内部改变
        OCISession	        *m_SessionHandle;               //OCI用户会话句柄
        OCISvcCtx	        *m_SvcHandle;                   //OCI服务环境句柄
        OCITrans            *m_TransHandle;                 //OCI明确手动开启的事务句柄

        conn_t (const conn_t&);
        conn_t& operator = (const conn_t&);
    };
}

#endif
