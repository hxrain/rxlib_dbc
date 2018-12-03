#ifndef _RX_DBC_SQL_PARAM_PARSE_H_
#define _RX_DBC_SQL_PARAM_PARSE_H_

#include "rx_str_util_ex.h"
#include "rx_str_util_tiny.h"

    //-----------------------------------------------------
    //SQL绑定参数名字解析器
    template<uint16_t max_param_count = 128>
    class sql_param_parse_t
    {
    public:
        //-------------------------------------------------
        //解析结果段
        typedef struct param_seg_t
        {
            const char *name;                              //绑定参数的名字
            uint16_t    name_len;                          //名字长度
        }param_seg_t;

        //-------------------------------------------------
        param_seg_t segs[max_param_count];                 //解析结果数组
        const char* str;
        uint16_t    count;

        //-------------------------------------------------
        sql_param_parse_t():str(NULL),count(0) {}
        //-------------------------------------------------
        //解析oracle的sql语句.绑定参数的格式为":name"
        //返回值:NULL成功;其他为语句错误点
        const char* ora_sql(const char* sql)
        {
            str = sql;
            count = 0;
            memset(segs,0,sizeof(segs));

            if (is_empty(sql)) return NULL;
            if (*sql == ':') return sql;

            //select id,'b','"',':g:":H:":i:',"STR",':":a":":INT"',UINT from tmp_dbc;

            //当前的引号深度
            uint16_t quote_deep = 0;
            char     quotes[20]="";

            bool    in_seg = false;

            char c = *sql;
            do
            {//逐字符分析
                param_seg_t &seg = segs[count];
                switch (c)
                {
                    case '\'':
                    case '\"':
                    {//碰到单引号或双引号了,需要进行深度的进出匹配
                        if (quote_deep&&quotes[quote_deep - 1] == c)
                            --quote_deep;   //遇到引号匹配的后一个则层级降低
                        else if (quote_deep >= 2 && quotes[quote_deep - 2] == c)
                            quote_deep -= 2;//遇到引号包裹了
                        else
                            quotes[quote_deep++] = c;

                        if (in_seg)
                            return sql;     //分段处理中遇到引号了,语法错误

                        break;
                    }
                    case ':':
                    {//碰到关键字符,冒号了,需要判断是否在引号中
                        if (!quote_deep)
                        {
                            if (strchr("+-*/< >,(=%", *(sql - 1)) == NULL)
                                return sql; //冒号的前面不是一个有效字符,也认为是错误
                            in_seg = true;  //不在引号中,遇到冒号了,认为进入了分段处理中
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
                    {//碰到结束字符了
                        if (in_seg)
                        {//如果是在分段处理中,则分段结束
                            if (seg.name_len < 2)
                                return sql; //如果分段只有一个冒号,或者名字太短,那么也是错误的

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
                            ++seg.name_len; //如果处于分段处理中,则增加分段长度
                    }
                }
                
                c=*(++sql);                 //取下一个字符

            } while (1);
            return NULL;
        }
        //-------------------------------------------------
        //解析oracle的sql绑定语句并转化为mysql的?格式.绑定参数的格式为":name"
        //返回值:buffsize缓冲器不足;其他为缓冲器结果串长度
        uint32_t ora2mysql(const char* sql,char* buff,uint32_t buffsize)
        {
            ora_sql(sql);                                   //先进行ora模式的参数解析
            uint32_t rc=0;
            if (count==0)
            {//如果解析后没有发现参数,则将原串直接返回,作为mysql的语句
                rc=rx::st::strcpy(buff,buffsize,sql);
                return rc==0?buffsize:rc;
            }

            //现在需要将ora的sql参数绑定格式转换为mysql格式
            rx::tiny_string_t<> cat(buffsize, buff);        //绑定输出缓冲区
            for(uint32_t i=0;i<count;++i)
            {//对参数段进行循环
                uint32_t len=uint32_t(segs[i].name-sql);    //获取参数段之前的串长度
                cat(sql,len);                               //复制参数段之前的内容
                cat<<'?';                                   //参数段的部分用?代替
                sql+=len+segs[i].name_len;                  //原语句跳过当前段,准备处理下一段
            }

            cat<<sql;                                       //全部参数段都处理完成后,拼装最后剩余的部分
            return cat.size() == cat.capacity() ? buffsize : cat.size();
        }
    };











#endif