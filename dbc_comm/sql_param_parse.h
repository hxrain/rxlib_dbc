#ifndef _RX_DBC_SQL_PARAM_PARSE_H_
#define _RX_DBC_SQL_PARAM_PARSE_H_

#include "rx_str_util_ex.h"
#include "rx_str_util_tiny.h"

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
        //����oracle��sql����䲢ת��Ϊmysql��?��ʽ.�󶨲����ĸ�ʽΪ":name"
        //����ֵ:buffsize����������;����Ϊ���������������
        uint32_t ora2mysql(const char* sql,char* buff,uint32_t buffsize)
        {
            ora_sql(sql);                                   //�Ƚ���oraģʽ�Ĳ�������
            uint32_t rc=0;
            if (count==0)
            {//���������û�з��ֲ���,��ԭ��ֱ�ӷ���,��Ϊmysql�����
                rc=rx::st::strcpy(buff,buffsize,sql);
                return rc==0?buffsize:rc;
            }

            //������Ҫ��ora��sql�����󶨸�ʽת��Ϊmysql��ʽ
            rx::tiny_string_t<> cat(buffsize, buff);        //�����������
            for(uint32_t i=0;i<count;++i)
            {//�Բ����ν���ѭ��
                uint32_t len=uint32_t(segs[i].name-sql);    //��ȡ������֮ǰ�Ĵ�����
                cat(sql,len);                               //���Ʋ�����֮ǰ������
                cat<<'?';                                   //�����εĲ�����?����
                sql+=len+segs[i].name_len;                  //ԭ���������ǰ��,׼��������һ��
            }

            cat<<sql;                                       //ȫ�������ζ�������ɺ�,ƴװ���ʣ��Ĳ���
            return cat.size() == cat.capacity() ? buffsize : cat.size();
        }
    };











#endif