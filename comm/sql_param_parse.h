#ifndef _RX_DBC_SQL_PARAM_PARSE_H_
#define _RX_DBC_SQL_PARAM_PARSE_H_

#include "rx_str_util.h"

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
                            if (strchr("+-*/< >,(=", *(sql - 1)) == NULL)
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
    };











#endif