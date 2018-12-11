#ifndef _RX_DBC_SQL_PARAM_PARSE_H_
#define _RX_DBC_SQL_PARAM_PARSE_H_

    //-----------------------------------------------------
    //SQL�󶨲������ֽ�����
    template<uint16_t max_param_count = 128>
    class sql_param_parse_t
    {
    public:
        //-------------------------------------------------
        //���������
        typedef struct param_seg_t
        {
            const char *name;                              //�󶨲���������
            uint16_t    name_len;                          //���ֳ���
        }param_seg_t;

        //-------------------------------------------------
        param_seg_t segs[max_param_count];                 //�����������
        const char* str;
        uint16_t    count;

        //-------------------------------------------------
        sql_param_parse_t():str(NULL),count(0) {}
        //-------------------------------------------------
        //����oracle��sql���.�󶨲����ĸ�ʽΪ":name"
        //����ֵ:NULL�ɹ�;����Ϊ�������
        const char* ora_sql(const char* sql)
        {
            str = sql;
            count = 0;
            memset(segs,0,sizeof(segs));

            if (is_empty(sql)) return NULL;
            if (*sql == ':') return sql;

            //select id,'b','"',':g:":H:":i:',"STR",':":a":":INT"',UINT from tmp_dbc;

            //��ǰ���������
            uint16_t quote_deep = 0;
            char     quotes[20]="";

            bool    in_seg = false;

            char c = *sql;
            do
            {//���ַ�����
                param_seg_t &seg = segs[count];
                switch (c)
                {
                    case '\'':
                    case '\"':
                    {//���������Ż�˫������,��Ҫ������ȵĽ���ƥ��
                        if (quote_deep&&quotes[quote_deep - 1] == c)
                            --quote_deep;   //��������ƥ��ĺ�һ����㼶����
                        else if (quote_deep >= 2 && quotes[quote_deep - 2] == c)
                            quote_deep -= 2;//�������Ű�����
                        else
                            quotes[quote_deep++] = c;

                        if (in_seg)
                            return sql;     //�ֶδ���������������,�﷨����

                        break;
                    }
                    case ':':
                    {//�����ؼ��ַ�,ð����,��Ҫ�ж��Ƿ���������
                        if (!quote_deep)
                        {
                            if (strchr("+-*/< >,(=%", *(sql - 1)) == NULL)
                                return sql; //ð�ŵ�ǰ�治��һ����Ч�ַ�,Ҳ��Ϊ�Ǵ���
                            in_seg = true;  //����������,����ð����,��Ϊ�����˷ֶδ�����
                            seg.name = sql;
                            seg.name_len = 1;
                        }
                        break;
                    }
                    case ',':
                    case ';':
                    case ')':
                    case '+':
                    case '-':
                    case '*':
                    case '/':
                    case '<':
                    case '>':
                    case '%':
                    case ' ':
                    case '\0':
                    {//���������ַ���
                        if (in_seg)
                        {//������ڷֶδ�����,��ֶν���
                            if (seg.name_len < 2)
                                return sql; //����ֶ�ֻ��һ��ð��,��������̫��,��ôҲ�Ǵ����

                            in_seg = false;
                            ++count;
                        }
                        if (c == 0)
                            return NULL;
                        break;
                    }
                    default:
                    {
                        if (in_seg)
                            ++seg.name_len; //������ڷֶδ�����,�����ӷֶγ���
                    }
                }

                c=*(++sql);                 //ȡ��һ���ַ�

            } while (1);
            return NULL;
        }
        //-------------------------------------------------
        //����oracle��sql����䲢ת��Ϊmysql��?��ʽ��buff[buffsize]��.�󶨲����ĸ�ʽΪ":name"
        //����ֵ:buffsize����������;����Ϊ���������������
        uint32_t ora2mysql(const char* sql,char* buff,uint32_t buffsize)
        {
            rx::tiny_string_t<> dst(buffsize, buff);
            return ora2mysql(sql,dst) ? dst.size(): buffsize;
        }
        bool ora2mysql(const char* sql, rx::tiny_string_t<>& dst)
        {
            ora_sql(sql);                                   //�Ƚ���oraģʽ�Ĳ�������
            uint32_t sql_len = rx::st::strlen(sql);

            if (count == 0)
                return dst.assign(sql, sql_len) == sql_len; //���������û�з��ֲ���,��ԭ��ֱ�ӷ���,��Ϊmysql�����

            //������Ҫ��ora��sql�����󶨸�ʽת��Ϊmysql��ʽ
            for (uint32_t i = 0; i<count; ++i)
            {//�Բ����ν���ѭ��
                uint32_t len = uint32_t(segs[i].name - sql);//��ȡ������֮ǰ�Ĵ�����
                dst(len, sql);                              //���Ʋ�����֮ǰ������
                dst << '?';                                 //�����εĲ�����?����
                sql += len + segs[i].name_len;              //ԭ���������ǰ��,׼��������һ��
            }

            dst << sql;                                     //ȫ�������ζ�������ɺ�,ƴװ���ʣ��Ĳ���
            return dst.size() != dst.capacity();
        }
    };

    //-------------------------------------------------
    //��ȡ�������
    inline sql_type_t get_sql_type(const char* SQL)
    {
        if (is_empty(SQL))
            return ST_UNKNOWN;

        while (*SQL)
        {
            if (*SQL == ' ')
                ++SQL;
            else
                break;
        }

        char tmp[5]; tmp[4] = 0;
        rx::st::strncpy(tmp, SQL, 4);
        rx::st::strupr(tmp);

        switch (*(uint32_t*)tmp)
        {
        case 0x454c4553:return ST_SELECT;
        case 0x41445055:return ST_UPDATE;
        case 0x45535055:return ST_UPDATE;
        case 0x454c4544:return ST_DELETE;
        case 0x45534e49:return ST_INSERT;
        case 0x41455243:return ST_CREATE;
        case 0x504f5244:return ST_DROP;
        case 0x45544c41:return ST_ALTER;
        case 0x49474542:return ST_BEGIN;
        case 0x20544553:return ST_SET;
        default:return ST_UNKNOWN;
        }
    }
#endif
