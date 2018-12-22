#ifndef _RX_DBC_SQL_PARAM_PARSE_H_
#define _RX_DBC_SQL_PARAM_PARSE_H_

    //-----------------------------------------------------
    //SQL绑定参数名字解析器
    template<uint16_t max_param_count = 64>
    class sql_param_parse_t
    {
    public:
        //-------------------------------------------------
        //解析结果段
        typedef struct param_seg_t
        {
            const char *name;                               //绑定参数的名字与类型串,ora与通用格式为":iInt";pg为"$1::int4".
            uint8_t    length;                              //名字类型串的整体长度
            uint8_t    offset;                              //对于pgsql参数"$1::int4"来说,告知类型字符串在整体中的偏移位置
        }param_seg_t;

        //-------------------------------------------------
        param_seg_t segs[max_param_count];                 //解析结果数组
        const char* str;
        uint16_t    count;

        //-------------------------------------------------
        void reset()
        {
            str = NULL;
            count = 0;
        }
        //-------------------------------------------------
        sql_param_parse_t():str(NULL),count(0) {}
        //-------------------------------------------------
        //解析oracle的sql语句.绑定参数的格式为":name"
        //返回值:NULL成功;其他为语句错误点
        const char* ora_sql(const char* sql)
        {
            reset();
            str = sql;
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
                            seg.length = 1;
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
                            if (seg.length < 2)
                                return sql; //如果分段只有一个冒号,或者名字太短,那么也是错误的

                            in_seg = false;
                            ++count;
                        }
                        if (c == 0)
                        {//真正结束了
                            return NULL;
                        }
                        break;
                    }
                    default:
                    {
                        if (in_seg)
                            ++seg.length; //如果处于分段处理中,则增加分段长度
                    }
                }

                c=*(++sql);                 //取下一个字符
            } while (1);
            return NULL;
        }
        //-------------------------------------------------
        //解析oracle的sql绑定语句并转化为mysql的?格式到buff[buffsize]中.绑定参数的格式为":name"
        //返回值:buffsize缓冲器不足;其他为缓冲器结果串长度
        uint32_t ora2mysql(const char* sql,char* buff,uint32_t buffsize)
        {
            rx::tiny_string_t<> dst(buffsize, buff);
            return ora2mysql(sql,dst) ? dst.size(): buffsize;
        }
        bool ora2mysql(const char* sql, rx::tiny_string_t<>& dst)
        {
            ora_sql(sql);                                   //先进行ora模式的参数解析
            uint32_t sql_len = rx::st::strlen(sql);

            if (count == 0)
                return dst.assign(sql, sql_len) == sql_len; //如果解析后没有发现参数,则将原串直接返回,作为mysql的语句

            //现在需要将ora的sql参数绑定格式转换为mysql格式
            for (uint32_t i = 0; i<count; ++i)
            {//对参数段进行循环
                uint32_t len = uint32_t(segs[i].name - sql);//获取参数段之前的串长度
                dst(len, sql);                              //复制参数段之前的内容
                dst << '?';                                 //参数段的部分用?代替
                sql += len + segs[i].length;              //原语句跳过当前段,准备处理下一段
            }

            dst << sql;                                     //全部参数段都处理完成后,拼装最后剩余的部分
            return dst.size() != dst.capacity();
        }
        //-------------------------------------------------
        //解析pg的sql语句.绑定参数的格式为"$n"或"$n::type"
        //返回值:NULL成功;其他为语句错误点
        const char* pg_sql(const char* sql)
        {
            str = sql;
            reset();
            memset(segs, 0, sizeof(segs));

            if (is_empty(sql)) return NULL;
            if (*sql == ':') return sql;

            //select id,UINT from tmp_dbc where UINT=$1 or UINT=$2::int8

            //当前的引号深度
            uint16_t quote_deep = 0;
            char     quotes[20] = "";

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
                    case '$':
                    {//碰到关键字符,$,需要判断是否在引号中
                        if (!quote_deep)
                        {
                            if (strchr("+-*/< >,(=%", *(sql - 1)) == NULL)
                                return sql; //$的前面不是一个有效字符,也认为是错误
                            in_seg = true;  //不在引号中,遇到$了,认为进入了分段处理中
                            seg.name = sql;
                            seg.length = 1;
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
                            if (seg.length < 2)
                                return sql; //如果分段只有一个$,或者名字太短,那么也是错误的

                            in_seg = false;
                            ++count;
                            //尝试找到pg参数名字后面附带的类型串的位置
                            uint32_t pos = rx::st::strnchr(seg.name, seg.length, ':');
                            if (pos != seg.length)
                            {
                                if (seg.name[pos + 1] == ':')
                                    seg.offset = pos + 2;
                            }
                        }
                        if (c == 0)
                            return NULL;

                        break;
                    }
                    default:
                    {
                        if (in_seg)
                            ++seg.length; //如果处于分段处理中,则增加分段长度
                    }
                }

                c = *(++sql);                 //取下一个字符

            } while (1);
            return NULL;
        }
        //-------------------------------------------------
        //解析oracle的sql绑定语句并转化为pg的$n格式到buff[buffsize]中.绑定参数的格式为":name"
        //返回值:buffsize缓冲器不足;其他为缓冲器结果串长度
        uint32_t ora2pgsql(const char* sql, char* buff, uint32_t buffsize)
        {
            rx::tiny_string_t<> dst(buffsize, buff);
            return ora2pgsql(sql, dst) ? dst.size() : buffsize;
        }
        bool ora2pgsql(const char* sql, rx::tiny_string_t<>& dst)
        {
            ora_sql(sql);                                   //先进行ora模式的参数解析
            uint32_t sql_len = rx::st::strlen(sql);

            if (count == 0)
                return dst.assign(sql, sql_len) == sql_len; //如果解析后没有发现参数,则将原串直接返回,作为mysql的语句

            rx::n2s_t n2s;

            //现在需要将ora的sql参数绑定格式转换为pgsql格式
            for (uint32_t i = 0; i<count; ++i)
            {//对参数段进行循环
                uint32_t len = uint32_t(segs[i].name - sql);//获取参数段之前的串长度
                dst(len, sql);                              //复制参数段之前的内容
                dst << '$' << n2s(i+1);                     //参数段的部分用?代替
                sql += len + segs[i].length;                //原语句跳过当前段,准备处理下一段
            }

            dst << sql;                                     //全部参数段都处理完成后,拼装最后剩余的部分
            return dst.size() != dst.capacity();
        }
    };

    //-------------------------------------------------
    //获取语句类型
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
        void *ptr=(void*)tmp;
        switch (*((uint32_t*)ptr))
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
        case 0x43544546:return ST_FETCH;
        default:return ST_UNKNOWN;
        }
    }
#endif
