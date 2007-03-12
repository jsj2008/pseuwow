#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <cctype>
#include "DefScriptDefines.h"
#include "DefScriptTools.h"


std::string DefScriptTools::stringToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), std::tolower);
    return s;
}

std::string DefScriptTools::stringToUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), std::toupper);
    return s;
}

std::string DefScriptTools::toString(ldbl num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}

std::string DefScriptTools::toString(uint64 num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}
// convert a string into ldbl
// valid input formats:
// normal numbers: 5439
// hex numbers: 0xa56ff, 0XFF, 0xDEADBABE, etc (must begin with 0x)
// float numbers: 99.65, 0.025
// negative numbers: -100, -0x3d, -55.123
ldbl DefScriptTools::toNumber(std::string str)
{
    ldbl num=0;
    uint64 u=0;
    bool negative=false;
    if(str.empty())
        return 0;
    if(str[0]=='-')
    {
        str.erase(0,1);
        negative=true;
    }
    unsigned int ppos=str.find('.');
    str=stringToUpper(str);

    if(str.length() > 2 && str[0]=='0' && str[1]=='X')
    {
        std::string lo(str.c_str()+2);
        std::string hi;
        while(lo.length()>8)
        {
            hi+=lo.at(0);
            lo.erase(0,1);
        }
        unsigned int hibits,lobits;
        hibits=strtoul(hi.c_str(),NULL,16);
        lobits=strtoul(lo.c_str(),NULL,16);
        u|=hibits;
        u<<=32;
        u|=lobits;
    }
    else
        u = atoi64(str.c_str());

    if(ppos!=std::string::npos)
    {
        std::string mantissa("0.");
        mantissa+=str.c_str()+ppos;
        num=(ldbl)strtod(mantissa.c_str(),NULL);
    }
    num=(unsigned long double)num + u;

    if(negative)
        num = -num;
    return num;
}

bool DefScriptTools::isTrue(std::string s)
{
    if(s.empty() || s=="false" || s=="0")
        return false;
    return true;
}

uint64 DefScriptTools::toUint64(std::string str)
{
    bool negative=false;
    uint64 num;
    if(str.empty())
        return 0;
    if(str[0]=='-')
    {
        str.erase(0,1);
        negative=true;
    }
    str=stringToUpper(str);
    if(str.length() > 2 && str[0]=='0' && str[1]=='X')
    {
        std::string lo(str.c_str()+2);
        std::string hi;
        while(lo.length()>8)
        {
            hi+=lo.at(0);
            lo.erase(0,1);
        }
        unsigned int hibits,lobits;
        hibits=strtoul(hi.c_str(),NULL,16);
        lobits=strtoul(lo.c_str(),NULL,16);
        num|=hibits;
        num<<=32;
        num|=lobits;
    }
    else
        num = atoi64(str.c_str());
    if(negative)
        num = (uint64)(-1) - num;
    return num;
}

uint64 DefScriptTools::atoi64(const char *str)
{
#if COMPILER == COMPILER_MICROSOFT
    __int64 tmp = _atoi64(str);
    return tmp;
#else
#error "Fix Me!"
    //return _atoi64(str);
#endif
}
