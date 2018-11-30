
#ifndef	_RX_DBC_ORA_CONN_H_
#define	_RX_DBC_ORA_CONN_H_

namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //管理ORACLE的连接的功能对象
    class conn_t
    {
        //-------------------------------------------------
        //分配事务句柄
        sword m_trans_alloc()
        {
            if (m_handle_trans)
            {
                rx_alert("conn_t事务处理顺序错误!");
                m_trans_free();
            }
            sword result=OCIHandleAlloc(m_handle_env, (void **)&m_handle_trans,  OCI_HTYPE_TRANS, 0, 0);
            if (result!=OCI_SUCCESS) return result;
            return OCIAttrSet(m_handle_svc, OCI_HTYPE_SVCCTX, m_handle_trans, 0, OCI_ATTR_TRANS, m_handle_err);
        }
        //-------------------------------------------------
        //释放事务句柄
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
        //连接到Oracle服务器(出现不可处理问题时抛异常,可控问题时给出ora错误代码)
        //返回值:0正常;其他出现了可控问题(比如密码即将过期的提示)
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
            //初始化OCI环境,得到环境句柄
            sword result = OCIEnvNlsCreate (&m_handle_env,env_mode,NULL,DBC_ORA_Malloc,DBC_ORA_Realloc,DBC_ORA_Free,0,NULL,op.charset_id, op.charset_id);
            if (result!=OCI_SUCCESS) throw (error_info_t (DBEC_ENV_FAIL, __FILE__, __LINE__));

            //分配得到服务器句柄
            OCIHandleAlloc (m_handle_env,(void **) &m_handle_svr,OCI_HTYPE_SERVER,0,NULL);

            //分配得到错误句柄
            result = OCIHandleAlloc (m_handle_env,(void **) &m_handle_err,OCI_HTYPE_ERROR,0,NULL);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_env, __FILE__, __LINE__));

            //!!连接到服务器!!
            result = OCIServerAttach (m_handle_svr,m_handle_err,(text *) dblink,(ub4)strlen (dblink),OCI_DEFAULT);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //分配得到功能句柄
            result = OCIHandleAlloc (m_handle_env,(void **) &m_handle_svc,OCI_HTYPE_SVCCTX,0,NULL);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //绑定功能句柄关联服务器句柄
            result = OCIAttrSet (m_handle_svc,OCI_HTYPE_SVCCTX,m_handle_svr,sizeof (OCIServer *),OCI_ATTR_SERVER,m_handle_err);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //分配得到用户会话句柄
            result = OCIHandleAlloc (m_handle_env,(void **) &m_handle_session,OCI_HTYPE_SESSION,0,NULL);
            if (result!=OCI_SUCCESS) throw (error_info_t (result, m_handle_err, __FILE__, __LINE__));

            //绑定用户名与口令到用户会话句柄
            result = OCIAttrSet (m_handle_session,OCI_HTYPE_SESSION,(text *) login,(ub4)strlen (login),OCI_ATTR_USERNAME,m_handle_err);
            if (result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            result = OCIAttrSet (m_handle_session,OCI_HTYPE_SESSION,(text *) password,(ub4)strlen (password),OCI_ATTR_PASSWORD,m_handle_err);
            if (result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            sword ec = 0;
            //!!进行登录认证!!
            result = OCISessionBegin(m_handle_svc,m_handle_err,m_handle_session,OCI_CRED_RDBMS,OCI_DEFAULT);
            if (result == OCI_SUCCESS_WITH_INFO)
            {
                char tmp[128];
                get_last_error(ec, tmp, sizeof(tmp));
                ec = ec;
            }
            else if(result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            //绑定功能句柄关联会话句柄
            result = OCIAttrSet (m_handle_svc,OCI_HTYPE_SVCCTX,m_handle_session,sizeof (OCISession *),OCI_ATTR_SESSION,m_handle_err);
            if (result != OCI_SUCCESS) throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));

            //尝试设置会话本地语言
            if (!is_empty(op.language))
                exec("ALTER SESSION SET NLS_LANGUAGE='%s'", op.language);

            //尝试设置会话的本地日期格式
            if (!is_empty(op.date_format))
                exec("ALTER SESSION SET NLS_DATE_FORMAT='%s'", op.date_format);

            m_is_valid = true;
            return ec;
        }
        //-------------------------------------------------
        //关闭当前的连接,不会抛出异常
        bool close (void)
        {
            sword	result = OCI_SUCCESS;
            ub4 ec = 0;

            m_trans_free();                                 //尝试先释放事务句柄

            if (m_handle_session != NULL)                   //!!结束用户会话!!
                result = OCISessionEnd (m_handle_svc,m_handle_err,m_handle_session,OCI_DEFAULT);
            if (result != OCI_SUCCESS) ++ec;

            
            if (m_handle_svr)                               //!!断开服务器连接!!
                result = OCIServerDetach (m_handle_svr,m_handle_err,OCI_DEFAULT);
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_session != NULL)                   //释放会话句柄
                result = OCIHandleFree(m_handle_session, OCI_HTYPE_SESSION);
            m_handle_session = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_svc != NULL)                       //释放功能句柄
                result = OCIHandleFree (m_handle_svc,OCI_HTYPE_SVCCTX);
            m_handle_svc = NULL;                         
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_err != NULL)                       //释放错误句柄
                result = OCIHandleFree (m_handle_err,OCI_HTYPE_ERROR);
            m_handle_err = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_svr != NULL)                       //释放服务器句柄
                result = OCIHandleFree (m_handle_svr,OCI_HTYPE_SERVER);
            m_handle_svr = NULL;
            if (result != OCI_SUCCESS) ++ec;

            if (m_handle_env != NULL)                       //释放环境句柄
                result = OCIHandleFree (m_handle_env,OCI_HTYPE_ENV);
            m_handle_env = NULL;
            if (result != OCI_SUCCESS) ++ec;
                   
            m_is_valid = false;
            rx_assert(ec == 0);
            return ec == 0;
        }
        //-------------------------------------------------
        //执行一条没有结果返回(非SELECT)的sql语句
        //内部实现是建立了临时的OiCommand对象,对于频繁执行的动作不建议使用此函数
        void exec(const char *sql,...);
        //-------------------------------------------------
        //切换到指定的用户专属库
        void schema_to(const char *schema) { exec("ALTER SESSION SET CURRENT_SCHEMA = %s", schema); }
        //-------------------------------------------------
        //当前连接启动事务
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
        //提交当前事务
        void trans_commit (void)
        {
            rx_assert(m_is_valid);
            sword result=OCITransCommit(m_handle_svc,m_handle_err,OCI_DEFAULT);
            if (result != OCI_SUCCESS)
                throw (error_info_t(result, m_handle_err, __FILE__, __LINE__));
            m_trans_free();                                 //提交成功,释放事务句柄,否则回滚时释放
        }
        //-------------------------------------------------
        //回滚当前事务
        //返回值:操作结果(回滚本身出错时不再抛出异常)
        bool trans_rollback (sword *ec=NULL)
        {
            rx_assert(m_is_valid);
            sword result=OCITransRollback(m_handle_svc,m_handle_err,OCI_DEFAULT);
            if (result!=OCI_SUCCESS&&ec)
            {//可以查看错误原因,就不抛异常了
                char tmp[128];
                get_last_error(*ec, tmp, sizeof(tmp));
            }
            m_trans_free();                                 //释放事务句柄
            return result == OCI_SUCCESS;
        }
        //-------------------------------------------------
        //进行服务器ping检查,真实的判断连接是否有效.不会抛出异常
        bool ping()
        {
            if (m_handle_svc == NULL)
                return false;
            sword result = OCIPing(m_handle_svc, m_handle_err, OCI_DEFAULT);
            return result == OCI_SUCCESS;
        }
        //-------------------------------------------------
        //得到指定内容项的本地化配置信息(比如 OCI_NLS_LANGUAGE)
        //返回值:操作结果(出错时不再抛出异常)
        bool nls_info(ub2 InfoItem,char* Buf,ub4 BufSize, sword *ec = NULL)
        {
            rx_assert(m_is_valid);
            sword result=OCINlsGetInfo(m_handle_session,m_handle_err,(oratext*)Buf,BufSize,InfoItem);
            if (result != OCI_SUCCESS&&ec)
            {//可以查看错误原因,就不抛异常了
                char tmp[128];
                get_last_error(*ec, tmp, sizeof(tmp));
            }
            return result == OCI_SUCCESS;
        }
        //-------------------------------------------------
        //获取最后的oci错误号ec,与对应的错误描述
        //返回值:操作结果(出错时不再抛出异常)
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

        rx::mem_allotter_i &m_mem;                          //内存分配器
        bool		        m_is_valid;

        OCIEnv		        *m_handle_env;                  //OCI环境句柄
        OCIServer	        *m_handle_svr;                  //OCI服务器句柄
        mutable OCIError	*m_handle_err;	                //OCI错误句柄.可能这个句柄会被OCI内部改变
        OCISession	        *m_handle_session;              //OCI用户会话句柄
        OCISvcCtx	        *m_handle_svc;                  //OCI业务功能句柄
        OCITrans            *m_handle_trans;                //OCI明确手动开启的事务句柄

        conn_t (const conn_t&);
        conn_t& operator = (const conn_t&);
    };
}

#endif
