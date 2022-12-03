#ifndef __url_h__
#define __url_h__

enum {
    SCHEME_UNKNOWN = -1,
    SCHEME_NONE = 0,
    SCHEME_FILE = -2,

    SCHEME_AFP = 548,
    SCHEME_DICT = 2628,
    SCHEME_FTP = 21,
    SCHEME_GOPHER = 70,
    SCHEME_HTTP = 80,
    SCHEME_HTTPS = 443,
    SCHEME_NFS = 2049,
    SCHEME_NNTP = 119,
    SCHEME_SFTP = 115,
    SCHEME_SMB = 445,
    SCHEME_SSH = 22,
    SCHEME_TELNET = 23,
    SCHEME_TFTP = 69
};

typedef struct URLRange {
    int location;
    int length;
} URLRange;

enum {
    URLComponentScheme,
    URLComponentUser,
    URLComponentPassword,
    URLComponentHost,
    URLComponentPort,
    URLComponentPath,
    URLComponentParams,
    URLComponentQuery,
    URLComponentFragment,
    URLComponentPathAndQuery
};

typedef struct URLComponents {

    int schemeType;
    int portNumber;

    URLRange scheme;
    URLRange user;
    URLRange password;
    URLRange host;
    URLRange port;
    URLRange path;
    URLRange params;
    URLRange query;
    URLRange fragment;
    
    URLRange pathAndQuery;
     
} URLComponents;

int ParseURL(const char *url, int length, URLComponents *components);

char *URLComponentGetCMalloc(const char *url, URLComponents *, int);

int URLComponentGet(const char *url, URLComponents *, int, char *);
int URLComponentGetC(const char *url, URLComponents *, int, char *);
//int URLComponentGetGS(const char *url, URLComponents *, int, GSString255Ptr);


#endif
