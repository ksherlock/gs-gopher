#pragma optimize 79
#pragma noroot

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "url.h"


enum {
    kScheme,
    kUser,
    kPassword,
    kHost,
    kPort,
    kPath,
    kParams,
    kQuery,
    kFragment
};


extern void parse_scheme(const char *cp, unsigned size, URLComponents *c);

/*
 * scheme://username:password@domain:port/path;parms?query_string#fragment_id
 * 
 * 
 */

char *URLComponentGetCMalloc(
  const char *url, 
  URLComponents *components, 
  int type)
{
    URLRange *rangePtr;
    URLRange r;
    char *tmp;
    
    if (!url || !components) return NULL;

    if (type < URLComponentScheme || type > URLComponentPathAndQuery) 
      return NULL;

    rangePtr = &components->scheme;
    
    r = rangePtr[type];

    if (!r.length) return NULL;
    
    tmp = (char *)malloc(r.length + 1);
    if (!tmp) return NULL;
    
    memcpy(tmp, url + r.location, r.length);
    tmp[r.length] = 0;

    return tmp;
}

int URLComponentGetC(const char *url, URLComponents *components, int type, char *dest)
{
    URLRange *rangePtr;
    URLRange r;
    
    if (!url || !components) return -1;
    
    
    if (type < URLComponentScheme || type > URLComponentPathAndQuery) return -1;
    
    rangePtr = &components->scheme;
    
    r = rangePtr[type];

    if (!dest) return r.length;
    else
    {
        if (r.length)
        {
            memcpy(dest, url + r.location, r.length);
        }
        dest[r.length] = 0;
        
        return r.length;
    }
}

// pstring.
int URLComponentGet(const char *url, URLComponents *components, int type, char *dest)
{
    URLRange *rangePtr;
    URLRange r;
    
    if (!url || !components) return -1;
    
    
    if (type < URLComponentScheme || type > URLComponentPathAndQuery) return -1;
    
    rangePtr = &components->scheme;
    
    r = rangePtr[type];

    if (!dest) return r.length;
    else if (r.length > 255) return -1;
    else
    {
        dest[0] = r.length;
        if (r.length)
        {
            memcpy(dest + 1, url + r.location, r.length);
        }
        
        return r.length;
    }
}




int ParseURL(const char *url, int length, struct URLComponents *components)
{
    char c;
    int state;
    int hasScheme;
    int hasAt;
    
    int i, l;
    
    URLRange range = { 0, 0 };
    URLRange *rangePtr;
    
    
    if (!components || !url) return 0;

    rangePtr = &components->scheme;
    memset(components, 0, sizeof(URLComponents));   

    state = kScheme;
    
    // 1 check for a scheme.
    // [A-Za-z0-9.+-]+ ':'
    
    
    hasScheme = 0; 
    hasAt = 0;   
    range.location = 0;
    
    for(i = 0; i < length; ++i)
    {
        c = url[i];
        
        if (c == ':')
        {
            hasScheme = 1;
            break;
        }
        
        if (c >= 'a' && c <= 'z') continue;
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= '0' && c <= '9') continue;
        
        //if (isalnum(c)) continue;
        if (c == '.') continue;
        if (c == '+') continue;
        if (c == '-') continue;
        
        break;
    }
    
    if (hasScheme)
    {
        if (i == 0) return 0;
        range.length = i;

        components->scheme = range;
        
        parse_scheme(url, i, components);
        
        ++i; // skip the ':'        
    }
    else
    {
        state = kPath;
        i = 0;
    }

    // 2. check for //
    // //user:password@domain:port/
    // otherwise, it's a path
        
    if (strncmp(url + i, "//", 2) == 0)
    {
        state = kUser;
        i += 2;
    }
    else
    {
        state = kPath;
    }
    
    //range = (URLRange){ i, 0 };
    range.location = i; range.length = 0;
      
    for( ; i < length; ++i)
    {
        c = url[i];
            
        if ((c < 32) || (c & 0x80))
        {
            // goto invalid_char;
            return 0;
        }
    
        switch(c)
        {
        // illegal chars.
        // % # not allowed but allowed.
        
        case ' ':
        case '"':
        case '<':
        case '>':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '`':
        case '{':
        case '|':
        case '}':
        case 0x7f:
            // goto invalid_char;
            return 0;
            break;
        
        
        case ':':
            // username -> password
            // domain -> port
            // password -> error
            // port -> error
            // all others, no transition.
            switch(state)
            {
            case kUser:
            case kHost:
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                
                state++;
                break;
            
            
            case kPassword:
            case kPort:
                return 0;
            
            }
            break;


        case '@':
            // username -> domain
            // password -> domain
            // all others no transition.
            // path -> error?
            
            switch(state)
            {
            case kUser:
            case kPassword:
                hasAt = 1;
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                
                state = kHost;            
                break;
            }
            break;
            
        case '/':
            // domain -> path
            // port -> path
            
            // (must be switched to domain:port)
            // username -> path
            // password -> path
            // all others, no transition.
            
            // '/' is part of the filename.
            switch (state)
            {
            case kUser:
                hasAt = 1;        // close enough
                state = kHost;    // username was actually the domain. 
                                  // pass through.     
            case kPassword:       // need to switch to domain:port later.
            case kHost:
            case kPort:
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }          
                
                //range = (URLRange){ i, 0 }; // '/' is part of path.
                range.location = i; range.length = 0;
                
                state = kPath; 
                break;            
            }
            break;
        
        case ';':
            // path -> param
            // all others, no transition.
            
            if (state == kPath)
            {
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                state = kParams; 
            }
            break;  
            
        case '?':
            // path -> query
            // param -> query
            // all others, no transition.
                  
            if (state == kPath || state == kParams)
            {
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                
                state = kQuery;
            }
            break;
            
            
        case '#':
            // fragment -> no transition
            // everything else transitions to a fragment.
            // can immediately end parsing since we know the length...
            
            l = i - range.location;
            if (l > 0)
            {
                range.length = l;
                rangePtr[state] = range; 
            }
            
            state = kFragment;
            range.location = i + 1; range.length = 0;

            i = length; // break from loop.
            break;              
        }

    }
    // finish up the last portion.
    if (state)
    {
        l = i - range.location;
        if (l > 0)
        {
            range.length = l;
            rangePtr[state] = range; 
        }        
    }
    
    
    if (!hasAt)
    {
        // user:name was actually domain:port
        components->host = components->user;
        components->port = components->password;
        
        components->user.location = 0;
        components->user.length = 0;
        components->password.location = 0;
        components->password.length = 0;
    }
    
    // port number conversion.
    if (components->port.location)
    {
        const char *tmp = url + components->port.location;
        int p = 0;

        l = components->port.length;
        
        
        for (i = 0; i < l; ++i)
        {
            c = tmp[i];
            
            if ((c < '0') || (c > '9'))
            {
                p = 0;
                break;
            }
            // convert to number.
            c &= 0x0f;
            
            // p *= 10;
            // 10x = 2x + 8x = (x << 1) + (x << 3)
            if (p)
            {
                int p2;
                
                p2 = p << 1;
                p = p2 << 2;
                p = p + p2;
            }
            p += c; 
        }
        
        components->portNumber = p;
    }
    
    // path and query.
    // path;params?query
    range = components->path;
    if (range.length)
    {
        if (components->params.length)
            range.length += components->params.length + 1;
            
        if (components->query.length)
            range.length += components->query.length + 1;
        
        components->pathAndQuery = range;
    }
    
    return 1;

}

