#ifndef	_RX_DBC_ORA_STATEMENT_H_
#define	_RX_DBC_ORA_STATEMENT_H_


namespace rx_dbc_ora
{
    //-----------------------------------------------------
    //ִ��SQL���Ĺ�����
    //-----------------------------------------------------
    class command_t
    {
        command_t (const command_t&);
        command_t& operator = (const command_t&);
        friend class field_t;
    protected:
        typedef rx::alias_array_t<sql_param_t, FIELD_NAME_LENGTH> param_array_t;
        param_array_t		                m_params;	    //��������

        conn_t		                        &m_Conn;		//����������������ݿ����Ӷ���
        OCIStmt			                    *m_StmtHandle;  //���������OCI���
        sql_stmt_t	                        m_SqlType;      //��������ǰSQL��������
        String                              m_SQL;          //Ԥ����ʱ��¼�Ĵ�ִ�е�SQL���
        ub2                                 m_max_bulk_count; //�������������ύ���������

        bool			                    m_IsPrepared;   //��ǵ�ǰ����Ƿ��Ѿ���Ԥ�����ɹ���
        bool			                    m_IsExecuted;   //��ǵ�ǰ����Ƿ��Ѿ�����ȷִ�й���
        //-------------------------------------------------
        //�ͷ�ȫ���Ĳ���
        void m_bind_reset()
        {
            m_params.clear();
            m_max_bulk_count=1;
        }


    public:
        //-------------------------------------------------
        command_t (conn_t &use):m_Conn(use)
        {
            m_StmtHandle = NULL;
            m_IsPrepared = false;
            m_IsExecuted = false;
            m_SqlType = ST_UNKNOWN;
            m_max_bulk_count=1;
        }
        //-------------------------------------------------
        conn_t& conn()const{return m_Conn;}
        //-------------------------------------------------
        virtual ~command_t (){close ();}
        //-------------------------------------------------
        //�õ���������SQL���
        const char* SQL(){return m_SQL.c_str();}
        //-------------------------------------------------
        //Ԥ����һ��SQL���,�õ���Ҫ����Ϣ,֮����Խ��в�������
        void prepare (const char *SQL,int Len = -1)
        {
            rx_assert (!is_empty(SQL));
            sword result;
            m_IsPrepared = false;
            m_IsExecuted = false;
            m_bind_reset();                                    //�����ܶ�����,�󶨵Ĳ���Ҳ������
            if (m_StmtHandle==NULL)
            {//����SQL���ִ�о��,��ʼִ�л���close֮��ִ��
                result=OCIHandleAlloc (m_Conn.m_EnvHandle,(void **) &m_StmtHandle,OCI_HTYPE_STMT,0,NULL);
                if (result!=OCI_SUCCESS)
                    throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
            }

            if (Len == -1) Len = (int)strlen (SQL);              //����SQL���
            result = OCIStmtPrepare (m_StmtHandle,m_Conn.m_ErrHandle,(text *) SQL,Len,OCI_NTV_SYNTAX,OCI_DEFAULT);

            if (result == OCI_SUCCESS)
            {
                ub2	stmt_type = 0;                          //�õ�SQL��������
                result = OCIAttrGet (m_StmtHandle,OCI_HTYPE_STMT,&stmt_type,NULL,OCI_ATTR_STMT_TYPE,m_Conn.m_ErrHandle);
                m_SqlType = (sql_stmt_t) stmt_type;           //stmt_typeΪ0˵������Ǵ����
            }

            if (result == OCI_SUCCESS)
            {
                m_IsPrepared = true;
                m_IsExecuted = false;
                m_SQL=SQL;
            }
            else
            {
                close();
                throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
            }
        }


        //-------------------------------------------------
        //ִ�е�ǰԤ�����������,�����з��ؼ�¼���Ĵ���
        //���:��ǰʵ�ʰ󶨲�����������.
        void exec (ub2 BulkCount=0)
        {
            if (!m_IsPrepared) throw (rx_dbc_ora::error_info_t (EC_METHOD_ORDER, __FILE__, __LINE__,"SQL Is Not Prepared!"));

            rx_assert(BulkCount<=m_max_bulk_count);
            if (BulkCount==0) BulkCount=m_max_bulk_count;

            ub4 iters = (m_SqlType == ST_SELECT) ? 0 : BulkCount;
            sword result = OCIStmtExecute (
                         m_Conn.m_SvcHandle,
                         m_StmtHandle,
                         m_Conn.m_ErrHandle,
                         iters,	// number of iterations
                         0,		// starting index from which the data in an array bind is relevant
                         NULL,	// input snapshot descriptor
                         NULL,	// output snapshot descriptor
                         OCI_DEFAULT);

            if (result == OCI_SUCCESS)
                m_IsExecuted = true;
            else
                throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //Ԥ������ִ��ͬʱ����,�м�û�а󶨲����Ļ�����,�ʺϲ��󶨲��������
        void exec (const char *SQL,int Len = -1)
        {
            prepare(SQL,Len);
            exec ();
        }
        //-------------------------------------------------
        //�õ���һ�����ִ�к�Ӱ�������(select��Ч)
        ub4 row_count()
        {
            if (!m_IsExecuted) throw (rx_dbc_ora::error_info_t (EC_METHOD_ORDER, __FILE__, __LINE__,"SQL Is Not Executed!"));
            ub4 RC=0;
            sword result=OCIAttrGet(m_StmtHandle, OCI_HTYPE_STMT,&RC, 0, OCI_ATTR_ROW_COUNT, m_Conn.m_ErrHandle);
            if (result != OCI_SUCCESS)
                throw (rx_dbc_ora::error_info_t (result, m_Conn.m_ErrHandle, __FILE__, __LINE__));
            return RC;
        }
        //-------------------------------------------------
        //�󶨲�����ʼ��:���������������󳤶�;����������;
        //���Ҫʹ��Bulkģʽ��������,��ô�����ȵ��ô˺���,��֪ÿ��Bulk��Ԫ������
        void bulk_bind_begin(ub2 MaxBulkCount,ub2 ParamCount=0)
        {
            m_bind_reset();
            if (ParamCount&&!m_params.make(ParamCount))
                throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
            m_max_bulk_count=MaxBulkCount;
            if (m_max_bulk_count>MAX_BULK_COUNT)
                throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
        }
        //-------------------------------------------------
        //��һ��������������ǰ�����,�������������DT_UNKNOWN,����ݱ�����ǰ׺�����Զ��ֱ�.�����ַ�������,�������ò�������ĳߴ�
        //�������������׸�������ʱ���Ը���SQL����е�':'��������ȷ��
        sql_param_t &bind (const char *name,data_type_t type = DT_UNKNOWN,int MaxStringSize=MAX_OUTPUT_TEXT_BYTES)
        {
            rx_assert (!is_empty(name));
            char Tmp[200];
            rx::st::strlwr(name,Tmp);

            if (!m_params.capacity())
            {//֮ǰû�г�ʼ����,��ô���ھ͸���SQL�еĲ����������г�ʼ��.�жϲ��������ͼ򵥵�����':'������,����ֻ�಻��,�ǿ��Ե�
                ub4 PC=rx::st::count(m_SQL.c_str(),':');
                if (PC==0)
                    throw (rx_dbc_ora::error_info_t (EC_SQL_NOT_PARAM, __FILE__, __LINE__));
                    
                if (!m_params.make(PC))
                    throw (rx_dbc_ora::error_info_t (EC_NO_MEMORY, __FILE__, __LINE__));
            }

            if (m_params.index(Tmp)!= m_params.capacity())
                throw (rx_dbc_ora::error_info_t (EC_PARAMNAME_DUP, __FILE__, __LINE__));

            ub4 ParamIdx=m_params.size();
            m_params.bind(ParamIdx, Tmp);                   //�������������������ֽ��й���
            sql_param_t &Ret = m_params[ParamIdx];          //�õ���������
            Ret.bind(m_Conn,m_StmtHandle,name,type,MaxStringSize,m_max_bulk_count);  //�Բ���������б�Ҫ�ĳ�ʼ��
            return Ret;
        }
        //-------------------------------------------------
        //�󶨹��Ĳ�������
        ub4 params() { return m_params.size(); }
        //-------------------------------------------------
        //�������ֵõ���������
        sql_param_t& operator [] (const char *name)
        {
            char Tmp[200];
            rx::st::strlwr(name,Tmp);
            ub4 Idx=m_params.index(Tmp);
            if (Idx==m_params.capacity())
                throw (rx_dbc_ora::error_info_t (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__, name));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //�����������ʲ�������,������0��ʼ
        sql_param_t& operator [] (ub4 Idx)
        {
            if (Idx>=m_params.size())
                throw (rx_dbc_ora::error_info_t (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__));
            return m_params[Idx];
        }
        //-------------------------------------------------
        //�ͷ������,����󶨵Ĳ���
        void close (void)
        {
            m_bind_reset();
            if (m_StmtHandle)
            {
                OCIHandleFree (m_StmtHandle,OCI_HTYPE_STMT);//�ͷ�SQL�����
                m_StmtHandle = NULL;
            }
        }
    };

    //-----------------------------------------------------
    //�����ݿ����Ӷ������ֱ��ִ��SQL���ķ���,�õ���HOStmt����,������Ҫ����HOStmt����ĺ���
    inline void conn_t::exec (const char *sql_block,int sql_len)
    {
        rx_assert (!is_empty(sql_block));
        command_t st (*this);
        st.prepare(sql_block,sql_len);
        st.exec ();
    }

}


#endif
